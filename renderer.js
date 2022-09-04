const ipc = require("electron").ipcRenderer;

// close app
function closeApp(e) {
  e.preventDefault();
  ipc.send("close");
}

document.getElementById("closeBtn").addEventListener("click", closeApp);
document.getElementById("disagreeBtn").addEventListener("click", closeApp);

function minimizeApp(e) {
  e.preventDefault();
  ipc.send("minimize");
}

document.getElementById("minBtn").addEventListener("click", minimizeApp);

const PAGES = ["./step-0.html", "./step-1.html"];

function nextPage1(e) {
  e.preventDefault();
  ipc.send("nextPage1");
}

document.getElementById("nextBtn").addEventListener("click", nextPage1);
