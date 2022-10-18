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

document.getElementById("main").classList.add("is-loaded");
