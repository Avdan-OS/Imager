function download(url, callback, encoding){
  var request = http.get(url, function(response) {
      if (encoding){
          response.setEncoding(encoding);
      }
      var len = parseInt(response.headers['content-length'], 10);
      var body = "";
      var cur = 0;
      var obj = document.getElementById('js-progress');
      var total = len / 1048576; //1048576 - bytes in  1Megabyte

      response.on("data", function(chunk) {
          body += chunk;
          cur += chunk.length;
          obj.innerHTML = "Downloading " + (100.0 * cur / len).toFixed(2) + "% " + (cur / 1048576).toFixed(2) + " mb\r" + ".<br/> Total size: " + total.toFixed(2) + " mb";
      });

      response.on("end", function() {
          callback(body);
          obj.innerHTML = "Downloading complete";
      });

      request.on("error", function(e){
          console.log("Error: " + e.message);
      });

  });
};