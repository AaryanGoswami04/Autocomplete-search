// history.js - Search history and theme functionality
document.addEventListener("DOMContentLoaded", function () {
  const searchHistoryElement = document.getElementById("searchHistory");
  const historyCountElement = document.getElementById("historyCount");
  const refreshButton = document.getElementById("refreshHistory");
  const themeStylesheet = document.getElementById('theme-stylesheet');

  const username = sessionStorage.getItem('currentUser') || "guest";

  function applyTheme(themeName) {
    if (themeName) {
      themeStylesheet.href = `${themeName}.css`;
    }
  }

  function showLoading() {
    searchHistoryElement.innerHTML = '<div class="loading-message">Loading search history...</div>';
    historyCountElement.textContent = "Loading...";
  }

  function showError(message) {
    searchHistoryElement.innerHTML = `<div class="error-message">Error: ${message}</div>`;
    historyCountElement.textContent = "Error loading count";
  }

  function showNoHistory() {
    searchHistoryElement.innerHTML = '<div class="no-history-message">No search history found.</div>';
    historyCountElement.textContent = "0 searches recorded";
  }

  function loadSearchHistory() {
    showLoading();
    
    const historyUrl = `/cgi-bin/search.cgi?history=1&user=${encodeURIComponent(username)}`;
    
    fetch(historyUrl)
      .then(response => {
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        return response.json();
      })
      .then(data => {
        if (data.error) {
          showError(data.error);
          return;
        }

        if (!Array.isArray(data) || data.length === 0) {
          showNoHistory();
          return;
        }

        historyCountElement.textContent = `${data.length} recent searches`;

        let historyHTML = "";
        data.forEach(item => {
          historyHTML += `
            <div class="history-item" data-search-term="${item.search_term}" title="Click to search again">
              <span class="search-term">${item.search_term}</span>
              <span class="search-timestamp">${item.timestamp}</span>
            </div>
          `;
        });

        searchHistoryElement.innerHTML = historyHTML;

        searchHistoryElement.querySelectorAll('.history-item').forEach(item => {
          item.addEventListener('click', function() {
            const searchTerm = this.getAttribute('data-search-term');
            window.location.href = `index.html?query=${encodeURIComponent(searchTerm)}`;
          });
        });
      })
      .catch(error => {
        console.error("Error loading history:", error);
        showError(error.message);
      });
  }
  
  function init() {
    // This function ensures the theme is set *before* content is loaded.
    if (username !== 'guest') {
      // 1. Fetch the theme first
      fetch(`/cgi-bin/search.cgi?get_settings=1&user=${username}`)
        .then(res => res.ok ? res.json() : { theme: 'light' })
        .then(settings => {
          // 2. Apply the theme
          applyTheme(settings.theme);
          // 3. THEN load the history content
          loadSearchHistory();
        })
        .catch(err => {
          console.error("Failed to load theme, defaulting to light.css", err);
          applyTheme('light');
          loadSearchHistory();
        });
    } else {
        // For guests, just load history with default theme
        loadSearchHistory();
    }
  }

  refreshButton.addEventListener("click", loadSearchHistory);

  // Initialize the page
  init();
});
