document.addEventListener("DOMContentLoaded", function () {
  const searchBox = document.getElementById("searchBox");
  const suggestions = document.getElementById("suggestions");
  const uploadInput = document.getElementById("wordFile");

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
      if (!file) {
        console.log("[script-simplified.js] No file selected.");
        return;
      }

      console.log("[script-simplified.js] Uploading file:", file.name, "size:", file.size);

      const reader = new FileReader();
      reader.onload = function (e) {
        const fileContent = e.target.result;
        const uploadUrl = "/cgi-bin/search.cgi";
        const fullUrl = `${uploadUrl}?user=${encodeURIComponent(currentUsername)}&filename=${encodeURIComponent(file.name)}`;

        console.log("[script-simplified.js] POST to:", fullUrl);

        fetch(fullUrl, {
          method: "POST",
          body: fileContent,
          headers: { "Content-Type": "text/plain;charset=UTF-8" }
        })
          .then(response => {
            console.log("[script-simplified.js] Upload response status:", response.status, response.statusText);
            return response.text();
          })
          .then(text => {
            console.log("[script-simplified.js] Upload response:", text);
           // alert("File uploaded successfully!");
          })
          .catch(err => {
            console.error("[script-simplified.js] Upload error:", err);
            alert("Upload failed: " + err.message);
          });
      };

      reader.onerror = (e) => console.error("[script-simplified.js] FileReader error:", e);
      reader.readAsText(file);
    });
  }

  // Search Autocomplete Handler
  searchBox.addEventListener("input", function () {
    const query = searchBox.value.trim();

    if (query.length === 0) {
      suggestions.innerHTML = "";
      return;
    }

    const username = currentUsername || "guest";
    const searchUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(query)}&user=${encodeURIComponent(username)}&log=0`;

    console.log("[script-simplified.js] Search request:", searchUrl);

    fetch(searchUrl)
      .then(response => {
        console.log("[script-simplified.js] Search response status:", response.status, response.statusText);
        return response.text();
      })
      .then(data => {
        suggestions.innerHTML = "";

        const lines = data.split("\n").filter(line => line.startsWith(" - "));
        console.log("[script-simplified.js] Found", lines.length, "suggestions for:", query);

        lines.forEach(line => {
          const item = document.createElement("li");
          item.textContent = line.replace(" - ", "");

          item.addEventListener("click", () => {
            const chosen = item.textContent;
            console.log("[script-simplified.js] Suggestion clicked:", chosen);

            searchBox.value = chosen;
            suggestions.innerHTML = "";

            // Log the click
            const clickUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(chosen)}&user=${encodeURIComponent(username)}&log=1`;
            console.log("[script-simplified.js] Logging click:", clickUrl);

            fetch(clickUrl)
              .then(r => r.text())
              .then(t => {
                console.log("[script-simplified.js] Click logged successfully, response:", t);
              })
              .catch(err => {
                console.error("[script-simplified.js] Click log error:", err);
              });
          });

          suggestions.appendChild(item);
        });
      })
      .catch(error => {
        console.error("[script-simplified.js] Search error:", error);
      });
  });

  // Enter Key Handler
  searchBox.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      const term = searchBox.value.trim();
      if (!term) return;

      console.log("[script-simplified.js] Enter pressed with term:", term);

      const username = currentUsername || "guest";
      const enterUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(term)}&user=${encodeURIComponent(username)}&log=1`;

      console.log("[script-simplified.js] Logging enter search:", enterUrl);

      fetch(enterUrl)
        .then(r => r.text())
        .then(t => {
          console.log("[script-simplified.js] Enter search logged successfully, response:", t);
        })
        .catch(err => {
          console.error("[script-simplified.js] Enter log error:", err);
        });
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

  // Debug function
  window.debugUser = function() {
    console.log("Current username:", currentUsername);
    console.log("SessionStorage user:", sessionStorage.getItem('currentUser'));
  };

  console.log("[script-simplified.js] Initialization complete");
});
