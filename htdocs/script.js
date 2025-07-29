// script.js
document.addEventListener("DOMContentLoaded", function () {
  const searchBox = document.getElementById("searchBox");
  const suggestions = document.getElementById("suggestions");
  const uploadInput = document.getElementById("wordFile");

  // --- Page-level debug ---
  console.log("[script.js] Current URL:", window.location.href);
  console.log("[script.js] URL search params:", window.location.search);

  // --- Username flow ---
  const urlParams = new URLSearchParams(window.location.search);
  const loggedInUser = urlParams.get("user");
  console.log("[script.js] User from URL:", loggedInUser);

  let currentUsername = "guest";

  if (loggedInUser) {
    // User just logged in -> store & use
    localStorage.setItem("username", loggedInUser);
    currentUsername = loggedInUser;
    console.log("[script.js] Saved user to localStorage:", loggedInUser);

    // Optional: clean URL (remove ?user=...) for nicer UX
    const cleanUrl = window.location.pathname;
    window.history.replaceState({}, document.title, cleanUrl);
    console.log("[script.js] Cleaned URL to:", cleanUrl);
  } else {
    const storedUsername = localStorage.getItem("username");
    console.log("[script.js] User from localStorage:", storedUsername);
    if (storedUsername && storedUsername !== "guest") {
      currentUsername = storedUsername;
    }
  }

  console.log("[script.js] Final username being used (init):", currentUsername);
  if (currentUsername !== "guest") {
    console.log("[script.js] Authenticated user:", currentUsername);
  }

  // --- Upload .txt file -> POST /cgi-bin/search.cgi?upload=1 ---
  if (uploadInput) {
    uploadInput.addEventListener("change", function () {
      const file = this.files[0];
      if (!file) {
        console.log("[script.js] No file selected.");
        return;
      }

      console.log("[script.js] Uploading file:", file.name, "size:", file.size);

      const reader = new FileReader();
      reader.onload = function (e) {
        const fileContent = e.target.result;

        const uploadUrl = "/cgi-bin/search.cgi?upload=1";
        console.log("[script.js] POST:", uploadUrl);

        fetch(uploadUrl, {
          method: "POST",
          body: fileContent,
          headers: { "Content-Type": "text/plain;charset=UTF-8" }
        })
          .then(response => {
            console.log("[script.js] Upload response status:", response.status, response.statusText);
            return response.text().then(text => ({ ok: response.ok, text }));
          })
          .then(({ ok, text }) => {
            console.log("[script.js] Upload response text:", text);
            if (!ok) console.warn("[script.js] Upload returned non-OK response");

            // Re-trigger search to refresh suggestions after upload
            const prev = searchBox.value;
            searchBox.value = "";
            setTimeout(() => {
              searchBox.value = prev;
              searchBox.dispatchEvent(new Event("input"));
            }, 100);
          })
          .catch(err => console.error("[script.js] Upload fetch error:", err));
      };

      reader.onerror = (e) => console.error("[script.js] FileReader error:", e);
      reader.readAsText(file);
    });
  }

  // --- Typing in search box -> suggestions only (log=0) ---
  searchBox.addEventListener("input", function () {
    const query = searchBox.value;

    if (query.length === 0) {
      suggestions.innerHTML = "";
      console.log("[script.js] Empty query; cleared suggestions.");
      return;
    }

    // Re-read username at request time (prevents stale value)
    const username = localStorage.getItem("username") || currentUsername || "guest";
    console.log("[script.js] Making search request with username:", username, "query:", query);

    // IMPORTANT: log=0 while typing
    const searchUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(query)}&user=${encodeURIComponent(username)}&log=0`;
    console.log("[script.js] GET:", searchUrl);

    fetch(searchUrl)
      .then(response => {
        console.log("[script.js] Search response status:", response.status, response.statusText);
        return response.text();
      })
      .then(data => {
        console.log("[script.js] Search response text:\n", data);
        suggestions.innerHTML = "";

        const lines = data.split("\n").filter(line => line.startsWith(" - "));
        if (lines.length === 0) {
          console.log("[script.js] No suggestions returned for:", query);
        }

        lines.forEach(line => {
          const item = document.createElement("li");
          item.textContent = line.replace(" - ", "");

          item.addEventListener("click", () => {
            // Put chosen suggestion in the box
            const chosen = item.textContent;
            searchBox.value = chosen;
            suggestions.innerHTML = "";

            // IMPORTANT: log=1 on click
            const clickUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(chosen)}&user=${encodeURIComponent(username)}&log=1`;
            console.log("[script.js] GET (click log):", clickUrl);
            fetch(clickUrl)
              .then(r => r.text())
              .then(t => console.log("[script.js] Click logged OK. Server text:\n", t))
              .catch(err => console.error("[script.js] Click log error:", err));

            // Optionally re-trigger suggestions for the chosen term
            searchBox.dispatchEvent(new Event("input"));
          });

          suggestions.appendChild(item);
        });
      })
      .catch(error => {
        console.error("[script.js] Error fetching suggestions:", error);
      });
  });

  // OPTIONAL: log Enter key as a search too (without clicking)
  searchBox.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      const term = searchBox.value.trim();
      if (!term) return;

      const username = localStorage.getItem("username") || currentUsername || "guest";
      const enterUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(term)}&user=${encodeURIComponent(username)}&log=1`;
      console.log("[script.js] GET (enter log):", enterUrl);
      fetch(enterUrl)
        .then(r => r.text())
        .then(t => console.log("[script.js] Enter logged OK. Server text:\n", t))
        .catch(err => console.error("[script.js] Enter log error:", err));
    }
  });

  // Close suggestions when clicking outside
  document.addEventListener("click", function (e) {
    if (!searchBox.contains(e.target) && !suggestions.contains(e.target)) {
      if (suggestions.innerHTML.trim() !== "") {
        console.log("[script.js] Clicked outside; clearing suggestions.");
      }
      suggestions.innerHTML = "";
    }
  });

  // Close suggestions on Escape
  document.addEventListener("keydown", function (e) {
    if (e.key === "Escape" && suggestions.innerHTML.trim() !== "") {
      console.log("[script.js] Escape pressed; clearing suggestions.");
      suggestions.innerHTML = "";
    }
  });
});
