const DiscordRPC = require("discord-rpc");

const clientId = "1012098922652631062";
const rpc = new DiscordRPC.Client({ transport: "ipc" });
const startTimestamp = new Date();

async function setActivity() {
  rpc.setActivity({
    details: "Installing AvdanOS...",
    state: "Selecting installation options..",
    startTimestamp,
    largeImageKey: "defaultpfp",
    largeImageText: "Installing AvdanOS",
    instance: false,
  });
}

rpc.on("ready", () => {
  setActivity();
  setInterval(() => {
    setActivity();
  }, 15000);
});

rpc.login({ clientId }).catch(console.error);
