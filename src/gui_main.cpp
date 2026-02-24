#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/thread.h>
#include <wx/gauge.h>
#include <wx/choice.h>

extern "C" {
    #include "imager/iso_operations.h"
    #include "imager/progress.h"
    #include "imager/drive_list.h"
}

#include <string>
#include <vector>

// Custom event for progress updates
wxDEFINE_EVENT(wxEVT_FLASH_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FLASH_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FLASH_ERROR, wxThreadEvent);

class FlashingThread : public wxThread {
public:
    FlashingThread(const wxString& isoPath, const wxString& devPath, wxEvtHandler* handler)
        : wxThread(wxTHREAD_DETACHED), m_isoPath(isoPath), m_devPath(devPath), m_handler(handler) {}

protected:
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

        if (write_iso_to_device(m_isoPath.mb_str(), m_devPath.mb_str(), progress_cb) == 0) {
            off_t isoSize = 0;
            int iso_fd = open_iso_file(m_isoPath.mb_str(), &isoSize);
            if (iso_fd >= 0) {
                #ifdef _WIN32
                _close(iso_fd);
                #else
                close(iso_fd);
                #endif
                
                if (verify_device_against_iso(m_isoPath.mb_str(), m_devPath.mb_str(), isoSize, progress_cb) == 0) {
                    wxQueueEvent(m_handler, new wxThreadEvent(wxEVT_FLASH_COMPLETE));
                } else {
                    wxThreadEvent* event = new wxThreadEvent(wxEVT_FLASH_ERROR);
                    event->SetString("Verification Failed");
                    wxQueueEvent(m_handler, event);
                }
            } else {
                wxThreadEvent* event = new wxThreadEvent(wxEVT_FLASH_ERROR);
                event->SetString("Failed to open ISO for verification");
                wxQueueEvent(m_handler, event);
            }
        } else {
            wxThreadEvent* event = new wxThreadEvent(wxEVT_FLASH_ERROR);
            event->SetString("Flash Failed");
            wxQueueEvent(m_handler, event);
        }

        return (ExitCode)0;
    }

private:
    wxString m_isoPath;
    wxString m_devPath;
    wxEvtHandler* m_handler;
    static wxEvtHandler* g_currentHandler;
};

wxEvtHandler* FlashingThread::g_currentHandler = nullptr;

class MainFrame : public wxFrame {
public:
    MainFrame() : wxFrame(NULL, wxID_ANY, "AvdanOS Imager", wxDefaultPosition, wxSize(480, 480)) {
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
        driveSizer->Add(isoSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

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

    void OnToggleUsbOnly(wxCommandEvent& event) {
        RefreshDrives();
    }

    void OnRefreshDrives(wxCommandEvent& event) {
        RefreshDrives();
    }

    void OnSelectISO(wxCommandEvent& event) {
        wxFileDialog openFileDialog(this, _("Open ISO file"), "", "",
                                   "ISO files (*.iso)|*.iso|All files (*.*)|*.*", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL) return;
        m_isoText->SetValue(openFileDialog.GetPath());
    }

    void OnStartFlash(wxCommandEvent& event) {
        if (m_isoText->GetValue().IsEmpty() || m_devChoice->GetSelection() == wxNOT_FOUND || m_drivePaths.empty()) {
            wxMessageBox("Please select both an ISO file and a target device.", "Error", wxOK | wxICON_ERROR);
            return;
        }

        wxString devPath = m_drivePaths[m_devChoice->GetSelection()];

        int answer = wxMessageBox("This will ERASE ALL DATA on " + devPath + ". Continue?", 
                                 "WARNING", wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
        if (answer != wxYES) return;

        m_startBtn->Disable();
        m_selectBtn->Disable();
        m_refreshBtn->Disable();
        m_devChoice->Disable();
        m_gauge->SetValue(0);
        m_statusLabel->SetLabel("Initializing...");

        FlashingThread* thread = new FlashingThread(m_isoText->GetValue(), devPath, this);
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
        wxMessageBox("The ISO has been successfully written and verified.", "Success", wxOK | wxICON_INFORMATION);
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
    }

    wxTextCtrl* m_isoText;
    wxChoice* m_devChoice;
    wxCheckBox* m_listUsbOnly;
    wxButton* m_selectBtn;
    wxButton* m_refreshBtn;
    wxButton* m_startBtn;
    wxGauge* m_gauge;
    wxStaticText* m_statusLabel;
    std::vector<wxString> m_drivePaths;
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
