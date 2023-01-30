const flasher = require('etcher-cli');

const e = document.getElementById('drive-select');
const driveletter = e.options[e.selectedIndex].text;

function flash(drivelet, e) {
  e.preventDefault();
  flasher.execSync('start cmd.exe /K cd', drivelet);
}

document
    .getElementById('flashbtn')
    .addEventListener('click', flash(driveletter));
