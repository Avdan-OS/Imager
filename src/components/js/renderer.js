const ipc = require('electron').ipcRenderer;

// close app
function closeApp(e) {
  e.preventDefault();
  ipc.send('close');
}

document.getElementById('closeBtn').addEventListener('click', closeApp);
document.getElementById('disagreeBtn').addEventListener('click', closeApp);


document.getElementById('main').classList.add('is-loaded');
