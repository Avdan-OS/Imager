#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/thread.h>
#include <wx/gauge.h>
#include <wx/choice.h>

extern "C" {
    #include "imager/iso_operations.h"
    #include "imager/progress.h"
    #include "imager/drive_list.h"
    #include "imager/image_format.h"
    #include "imager/image_reader.h"
    #include "imager/image_write.h"
    #include "imager/windows_iso.h"
    #include "imager/device_lock.h"
}

#include <string>
#include <vector>

wxDEFINE_EVENT(wxEVT_FLASH_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FLASH_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FLASH_ERROR, wxThreadEvent);

class FlashingThread : public wxThread {
public:
    FlashingThread(const wxString& isoPath, const wxString& devPath,
                   image_format_t format, bool extractMode, wxEvtHandler* handler)
        : wxThread(wxTHREAD_DETACHED), m_isoPath(isoPath), m_devPath(devPath),
          m_format(format), m_extractMode(extractMode), m_handler(handler) {}

protected:
    void PostError(const wxString& msg) {
        wxThreadEvent* event = new wxThreadEvent(wxEVT_FLASH_ERROR);
        event->SetString(msg);
        wxQueueEvent(m_handler, event);
    }

    virtual ExitCode Entry() {
        g_currentHandler = m_handler;

        auto progress_cb = [](off_t done, off_t total, const char* label) {
            if (g_currentHandler) {
                wxThreadEvent* event = new wxThreadEvent(wxEVT_FLASH_PROGRESS);
                double progress = (total > 0) ? (double)done / total : 0;
                event->SetPayload(progress);
                event->SetString(wxString::FromUTF8(label));
                wxQueueEvent(g_currentHandler, event);
            }
        };

        device_lock_t* lock = device_lock_acquire(m_devPath.mb_str(),
                m_extractMode ? DEVICE_LOCK_UNMOUNT_ONLY : DEVICE_LOCK_EXCLUSIVE);
        if (lock == NULL) {
            PostError("Device is busy: could not unmount/lock it.\n"
                      "Close any programs using the drive and retry.");
            return (ExitCode)0;
        }

        if (m_extractMode) {
            if (write_iso_extracted(m_isoPath.mb_str(), m_devPath.mb_str(), progress_cb) == 0) {
                wxQueueEvent(m_handler, new wxThreadEvent(wxEVT_FLASH_COMPLETE));
            } else {
                PostError("Extraction Failed");
            }
        } else {
            off_t bytesWritten = 0;
            if (write_image_to_device(m_isoPath.mb_str(), m_format, m_devPath.mb_str(),
                                      progress_cb, &bytesWritten) == 0) {
                if (verify_device_against_image(m_isoPath.mb_str(), m_format, m_devPath.mb_str(),
                                                bytesWritten, progress_cb) == 0) {
                    wxQueueEvent(m_handler, new wxThreadEvent(wxEVT_FLASH_COMPLETE));
                } else {
                    PostError("Verification Failed");
                }
            } else {
                PostError("Flash Failed");
            }
        }

        device_lock_release(lock);
        return (ExitCode)0;
    }

private:
    wxString m_isoPath;
    wxString m_devPath;
    image_format_t m_format;
    bool m_extractMode;
    wxEvtHandler* m_handler;
    static wxEvtHandler* g_currentHandler;
};

wxEvtHandler* FlashingThread::g_currentHandler = nullptr;

class MainFrame : public wxFrame {
public:
    MainFrame() : wxFrame(NULL, wxID_ANY, "AvdanOS Imager", wxDefaultPosition, wxSize(480, 560)) {
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // --- Drive Properties ---
        wxStaticBoxSizer* driveSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Drive Properties");

        driveSizer->Add(new wxStaticText(this, wxID_ANY, "Device:"), 0, wxALL, 5);
        
        wxBoxSizer* devSizer = new wxBoxSizer(wxHORIZONTAL);
        m_devChoice = new wxChoice(this, wxID_ANY);
        devSizer->Add(m_devChoice, 1, wxEXPAND | wxRIGHT, 5);
        m_refreshBtn = new wxButton(this, wxID_ANY, "REFRESH", wxDefaultPosition, wxSize(80, -1));
        devSizer->Add(m_refreshBtn, 0);
        driveSizer->Add(devSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        
        m_listUsbOnly = new wxCheckBox(this, wxID_ANY, "List USB drives only");
        m_listUsbOnly->SetValue(true);
        driveSizer->Add(m_listUsbOnly, 0, wxLEFT | wxRIGHT | wxTOP, 10);
        
        driveSizer->Add(new wxStaticText(this, wxID_ANY, "Select the target USB drive."), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

        driveSizer->Add(new wxStaticText(this, wxID_ANY, "Boot Selection:"), 0, wxALL, 5);
        wxBoxSizer* isoSizer = new wxBoxSizer(wxHORIZONTAL);
        m_isoText = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        isoSizer->Add(m_isoText, 1, wxEXPAND | wxRIGHT, 5);
        m_selectBtn = new wxButton(this, wxID_ANY, "SELECT");
        isoSizer->Add(m_selectBtn, 0);
        driveSizer->Add(isoSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        m_formatLabel = new wxStaticText(this, wxID_ANY, "Format: (no image selected)");
        driveSizer->Add(m_formatLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);

        m_extractCheck = new wxCheckBox(this, wxID_ANY, "Windows extraction mode (FAT32 + file copy)");
        m_extractCheck->SetToolTip("For Windows-style install ISOs: partitions the drive,\n"
                                   "formats FAT32, copies files and splits install.wim\n"
                                   "when it exceeds the FAT32 4 GiB limit.");
        driveSizer->Add(m_extractCheck, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 10);

        mainSizer->Add(driveSizer, 0, wxEXPAND | wxALL, 15);

        // --- Status ---
        wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Status");
        
        m_gauge = new wxGauge(this, wxID_ANY, 100);
        statusSizer->Add(m_gauge, 0, wxEXPAND | wxALL, 10);
        
        m_statusLabel = new wxStaticText(this, wxID_ANY, "Ready");
        statusSizer->Add(m_statusLabel, 0, wxALIGN_CENTER | wxBOTTOM, 10);

        mainSizer->Add(statusSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

        // --- Footer ---
        mainSizer->AddStretchSpacer();
        m_startBtn = new wxButton(this, wxID_ANY, "START", wxDefaultPosition, wxSize(120, 40));
        mainSizer->Add(m_startBtn, 0, wxALIGN_RIGHT | wxALL, 20);

        SetSizer(mainSizer);

        // Events
        m_selectBtn->Bind(wxEVT_BUTTON, &MainFrame::OnSelectISO, this);
        m_refreshBtn->Bind(wxEVT_BUTTON, &MainFrame::OnRefreshDrives, this);
        m_listUsbOnly->Bind(wxEVT_CHECKBOX, &MainFrame::OnToggleUsbOnly, this);
        m_startBtn->Bind(wxEVT_BUTTON, &MainFrame::OnStartFlash, this);
        Bind(wxEVT_FLASH_PROGRESS, &MainFrame::OnProgress, this);
        Bind(wxEVT_FLASH_COMPLETE, &MainFrame::OnComplete, this);
        Bind(wxEVT_FLASH_ERROR, &MainFrame::OnError, this);

        RefreshDrives();
    }

private:
    void RefreshDrives() {
        m_devChoice->Clear();
        m_drivePaths.clear();

        drive_info_t drives[16];
        int count = list_available_drives(drives, 16);
        bool usbOnly = m_listUsbOnly->GetValue();
        int added = 0;
        
        for (int i = 0; i < count; i++) {
            if (usbOnly && !drives[i].is_removable) continue;

            m_devChoice->Append(wxString::FromUTF8(drives[i].name));
            m_drivePaths.push_back(wxString::FromUTF8(drives[i].path));
            added++;
        }

        if (added > 0) m_devChoice->SetSelection(0);
        else m_devChoice->Append("No suitable drives found");
    }

    void AnalyzeImage() {
        m_imgValid = false;
        m_formatLabel->SetLabel("Format: (no image selected)");
        if (m_isoText->GetValue().IsEmpty()) return;

        if (detect_image_format(m_isoText->GetValue().mb_str(), &m_imgInfo) != 0) {
            m_formatLabel->SetLabel("Format: could not analyze file");
            m_extractCheck->Enable(false);
            m_extractCheck->SetValue(false);
            return;
        }
        m_imgValid = true;

        wxString info = wxString::Format("Format: %s", image_format_name(m_imgInfo.format));
        if (m_imgInfo.volume_label[0] != '\0') {
            info += wxString::Format("  |  Label: %s", wxString::FromUTF8(m_imgInfo.volume_label));
        }
        m_formatLabel->SetLabel(info);

        bool compressed = (image_format_is_raw_writable(&m_imgInfo) < 0);
        m_extractCheck->Enable(!compressed);
        m_extractCheck->SetValue(!compressed && m_imgInfo.format == IMG_FORMAT_UDF);
        Layout();
    }

    void OnToggleUsbOnly(wxCommandEvent& event) {
        RefreshDrives();
    }

    void OnRefreshDrives(wxCommandEvent& event) {
        RefreshDrives();
    }

    void OnSelectISO(wxCommandEvent& event) {
        wxFileDialog openFileDialog(this, _("Open disk image"), "", "",
                                   "Disk images (*.iso;*.img;*.gz;*.xz;*.bz2;*.zst)|*.iso;*.img;*.gz;*.xz;*.bz2;*.zst|All files (*.*)|*.*",
                                   wxFD_OPEN|wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL) return;
        m_isoText->SetValue(openFileDialog.GetPath());
        AnalyzeImage();
    }

    void OnStartFlash(wxCommandEvent& event) {
        if (m_isoText->GetValue().IsEmpty() || m_devChoice->GetSelection() == wxNOT_FOUND || m_drivePaths.empty()) {
            wxMessageBox("Please select both an image file and a target device.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        if (!m_imgValid) {
            wxMessageBox("The selected image could not be analyzed.", "Error", wxOK | wxICON_ERROR);
            return;
        }

        bool extractMode = m_extractCheck->GetValue();
        int writable = image_format_is_raw_writable(&m_imgInfo);

        if (writable < 0) {
            if (!image_reader_format_supported(m_imgInfo.format)) {
                wxMessageBox(wxString::Format(
                        "This build has no %s support compiled in.\n"
                        "Decompress the file manually or rebuild with the codec library.",
                        image_format_name(m_imgInfo.format)),
                    "Error", wxOK | wxICON_ERROR);
                return;
            }
        }

        wxString devPath = m_drivePaths[m_devChoice->GetSelection()];

        wxString warning = "This will ERASE ALL DATA on " + devPath + ".";
        if (!extractMode && writable == 0) {
            warning += "\n\nWarning: " + wxString::FromUTF8(m_imgInfo.description);
            if (m_imgInfo.format == IMG_FORMAT_UDF || m_imgInfo.format == IMG_FORMAT_ISO9660) {
                warning += "\nTip: enable 'Windows extraction mode' for Windows install ISOs.";
            }
        }
        warning += "\n\nContinue?";

        int answer = wxMessageBox(warning, "WARNING", wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
        if (answer != wxYES) return;

        m_startBtn->Disable();
        m_selectBtn->Disable();
        m_refreshBtn->Disable();
        m_devChoice->Disable();
        m_extractCheck->Disable();
        m_gauge->SetValue(0);
        m_statusLabel->SetLabel("Initializing...");

        FlashingThread* thread = new FlashingThread(m_isoText->GetValue(), devPath,
                                                    m_imgInfo.format, extractMode, this);
        if (thread->Run() != wxTHREAD_NO_ERROR) {
            wxMessageBox("Could not create the flashing thread!", "Error", wxOK | wxICON_ERROR);
            ResetUI();
        }
    }

    void OnProgress(wxThreadEvent& event) {
        double progress = event.GetPayload<double>();
        m_gauge->SetValue((int)(progress * 100));
        m_statusLabel->SetLabel(event.GetString());
    }

    void OnComplete(wxThreadEvent& event) {
        m_gauge->SetValue(100);
        m_statusLabel->SetLabel("Flash Successful!");
        wxMessageBox("The image has been successfully written to the device.", "Success", wxOK | wxICON_INFORMATION);
        ResetUI();
    }

    void OnError(wxThreadEvent& event) {
        m_statusLabel->SetLabel("Error: " + event.GetString());
        wxMessageBox(event.GetString(), "Error", wxOK | wxICON_ERROR);
        ResetUI();
    }

    void ResetUI() {
        m_startBtn->Enable();
        m_selectBtn->Enable();
        m_refreshBtn->Enable();
        m_devChoice->Enable();
        bool compressed = m_imgValid && (image_format_is_raw_writable(&m_imgInfo) < 0);
        m_extractCheck->Enable(m_imgValid && !compressed);
    }

    wxTextCtrl* m_isoText;
    wxChoice* m_devChoice;
    wxCheckBox* m_listUsbOnly;
    wxCheckBox* m_extractCheck;
    wxButton* m_selectBtn;
    wxButton* m_refreshBtn;
    wxButton* m_startBtn;
    wxGauge* m_gauge;
    wxStaticText* m_statusLabel;
    wxStaticText* m_formatLabel;
    std::vector<wxString> m_drivePaths;
    image_info_t m_imgInfo;
    bool m_imgValid = false;
};

class ImagerApp : public wxApp {
public:
    virtual bool OnInit() {
        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ImagerApp);
