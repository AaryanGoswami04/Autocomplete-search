const searchBox = document.getElementById('searchBox');
const suggestions = document.getElementById('suggestions');

suggestions.addEventListener('mousedown', (e) => {
  e.preventDefault(); 
});

document.getElementById('wordFile').addEventListener('change', function () {
  const file = this.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = function (e) {
    const fileContent = e.target.result;

    fetch('/cgi-bin/search.cgi?upload=1', {
      method: 'POST',
      body: fileContent,
    })
    .then(response => response.text())
    .then(msg => {
      console.log(msg);

      const fakeInput = searchBox.value;
      searchBox.value = '';
      setTimeout(() => {
        searchBox.value = fakeInput;
        searchBox.dispatchEvent(new Event('input'));
      }, 100);
    });
  };
  reader.readAsText(file);
});

searchBox.addEventListener('input', function () {
  const query = searchBox.value;

  if (query.length === 0) {
    suggestions.innerHTML = "";
    return;
  }

  fetch(`/cgi-bin/search.cgi?query=${encodeURIComponent(query)}`)
    .then(response => response.text())
    .then(data => {
      suggestions.innerHTML = "";

      const lines = data.split("\n").filter(line => line.startsWith(" - "));
      lines.forEach(line => {
        const item = document.createElement("li");
        item.textContent = line.replace(" - ", "");

        item.addEventListener('click', () => {
          searchBox.value = item.textContent;
          suggestions.innerHTML = "";
          searchBox.dispatchEvent(new Event('input')); 
        });

        suggestions.appendChild(item);
      });
    })
    .catch(error => {
      console.error("Error:", error);
    });
});
