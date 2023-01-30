const spawn = require('child_process').spawn;

function listDrives() {
  const list = spawn('cmd');

  return new Promise((resolve, reject) => {
    list.stdout.on('data', function(data) {
      // console.log('stdout: ' + String(data));
      const output = String(data);
      const out = output
          .split('\r\n')
          .map((e) => e.trim())
          .filter((e) => e != '');
      if (out[0] === 'Name') {
        resolve(out.slice(1));
      }
      // console.log("stdoutput:", out)
    });

    list.stderr.on('data', function(data) {
      // console.log('stderr: ' + data);
    });

    list.on('exit', function(code) {
      console.log('child process exited with code ' + code);
      if (code !== 0) {
        reject(code);
      }
    });

    list.stdin.write('wmic logicaldisk get name\n');
    list.stdin.end();
  });
}

listDrives().then((data) => {
  return data;
}); /* console.log(data)*/
