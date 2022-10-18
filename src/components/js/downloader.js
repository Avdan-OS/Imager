const request = require("request"),
  fs = require("fs");

const download = (url, filename, callback) => {
  const file = fs.createWriteStream(filename);
  let receivedBytes = 0;

  var length;
  var testsomething;

  // Send request to the given URL
  request
    .get(url)
    .on("response", (response) => {
      if (response.statusCode !== 200)
        return callback("Response status was " + response.statusCode);

      const totalBytes = response.headers["content-length"];
      testsomething.push(totalBytes);
    })
    .on("data", (chunk) => {
      receivedBytes += chunk.length;
      length.push(receivedBytes);

      // calculates percentage
      const sum = length.reduce((a, b) => a + b, 0);
      const completedPercentage = (sum / testsomething) * 100;

      // sets the progress bar to the percentage
      setBar(completedPercentage);
    })
    .pipe(file)
    .on("error", (err) => {
      fs.unlink(filename);
    });

  file.on("finish", () => {
    file.close(callback);
  });

  file.on("error", (err) => {
    fs.unlink(filename);
    return callback(err.message);
  });
};

const fileUrl = `https://unsplash.com/photos/FHo4labMPSQ/download`;
download(fileUrl, "knot.jpg", () => {});
