document.addEventListener("DOMContentLoaded", function () {
  const searchBox = document.getElementById("searchBox");
  const suggestions = document.getElementById("suggestions");
  const uploadInput = document.getElementById("wordFile");
  const saveSearchButton = document.getElementById("saveSearchButton");

  // Username handling
  const urlParams = new URLSearchParams(window.location.search);
  const loggedInUser = urlParams.get("user");
  
  let currentUsername = loggedInUser || sessionStorage.getItem('currentUser') || "guest";

  if (loggedInUser) {
    sessionStorage.setItem('currentUser', loggedInUser);
    const cleanUrl = window.location.pathname;
    window.history.replaceState({}, document.title, cleanUrl);
  }

  // File Upload Handler
  if (uploadInput) {
    uploadInput.addEventListener("change", function () {
      const file = this.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = function (e) {
        const fileContent = e.target.result;
        const fullUrl = `/cgi-bin/search.cgi?user=${encodeURIComponent(currentUsername)}&filename=${encodeURIComponent(file.name)}`;

        fetch(fullUrl, {
          method: "POST",
          body: fileContent,
          headers: { "Content-Type": "text/plain;charset=UTF-8" }
        })
          .then(response => response.text())
          .then(text => console.log("[script.js] Upload response:", text))
          .catch(err => {
            console.error("[script.js] Upload error:", err);
            alert("Upload failed: " + err.message);
          });
      };
      reader.readAsText(file);
    });
  }

  // Search Autocomplete Handler
  searchBox.addEventListener("input", function () {
    const query = searchBox.value.trim();
    saveSearchButton.style.display = 'none';
    saveSearchButton.classList.remove('saved'); // Reset icon style

    if (query.length === 0) {
      suggestions.innerHTML = "";
      return;
    }

    const username = currentUsername || "guest";
    const searchUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(query)}&user=${encodeURIComponent(username)}&log=0`;

    fetch(searchUrl)
      .then(response => response.text())
      .then(data => {
        suggestions.innerHTML = "";
        const lines = data.split("\n").filter(line => line.startsWith(" - "));

        lines.forEach(line => {
          const item = document.createElement("li");
          item.textContent = line.replace(" - ", "");

          item.addEventListener("click", () => {
            const chosen = item.textContent;
            searchBox.value = chosen;
            suggestions.innerHTML = "";

            // (MODIFIED) Show the icon and ensure it's in the default state
            saveSearchButton.style.display = 'block';
            saveSearchButton.disabled = false;
            saveSearchButton.classList.remove('saved');

            const clickUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(chosen)}&user=${encodeURIComponent(username)}&log=1`;
            fetch(clickUrl).catch(err => console.error("[script.js] Click log error:", err));
          });
          suggestions.appendChild(item);
        });
      })
      .catch(error => console.error("[script.js] Search error:", error));
  });

  // Event listener for the save icon button
  saveSearchButton.addEventListener('click', function() {
    const termToSave = searchBox.value.trim();
    if (!termToSave) return;

    const username = currentUsername || "guest";
    const saveUrl = `/cgi-bin/search.cgi?user=${encodeURIComponent(username)}&term=${encodeURIComponent(termToSave)}&save_search=1`;
        
    fetch(saveUrl, { method: "POST" })
      .then(res => res.json())
      .then(data => {
        if(data.success) {
          // (MODIFIED) Add 'saved' class to change icon style and disable it
          saveSearchButton.classList.add('saved');
          saveSearchButton.disabled = true;
        } else {
          alert('Failed to save search: ' + (data.error || 'Unknown error'));
        }
      })
      .catch(err => {
        console.error("[script.js] Save error:", err);
        alert('An error occurred while saving.');
      });
  });

  // Enter Key Handler
  searchBox.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      const term = searchBox.value.trim();
      if (!term) return;
      const username = currentUsername || "guest";
      const enterUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(term)}&user=${encodeURIComponent(username)}&log=1`;
      fetch(enterUrl).catch(err => console.error("[script.js] Enter log error:", err));
    }
  });

  // Click outside to clear suggestions
  document.addEventListener("click", function (e) {
    if (!searchBox.contains(e.target) && !suggestions.contains(e.target)) {
      suggestions.innerHTML = "";
    }
  });

  // Escape key to clear suggestions
  document.addEventListener("keydown", function (e) {
    if (e.key === "Escape") {
      suggestions.innerHTML = "";
    }
  });
});
