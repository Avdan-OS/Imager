const flashButton = document.getElementById("flashbtn");
const toolButtons = document.querySelectorAll(".pg3btn");

toolButtons.addEventListener("focus", () => {
  flashButton.style.display = "block";
});

toolButtons.addEventListener("blur", () => {
  flashButton.style.display = "none";  
});