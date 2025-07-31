// history.js - Search history functionality
document.addEventListener("DOMContentLoaded", function () {
  const searchHistoryElement = document.getElementById("searchHistory");
  const historyCountElement = document.getElementById("historyCount");
  const refreshButton = document.getElementById("refreshHistory");

  // Get username from URL or session storage
  const urlParams = new URLSearchParams(window.location.search);
  const loggedInUser = urlParams.get("user");
  let currentUsername = loggedInUser || sessionStorage.getItem('currentUser') || "guest";
  
  if (loggedInUser) {
    sessionStorage.setItem('currentUser', loggedInUser);
    // Clean URL
    const cleanUrl = window.location.pathname;
    window.history.replaceState({}, document.title, cleanUrl);
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
    searchHistoryElement.innerHTML = '<div class="no-history-message">No search history found. Start searching to see your history here!</div>';
    historyCountElement.textContent = "0 searches recorded";
  }

  function loadSearchHistory() {
    showLoading();
    
    const username = currentUsername || "guest";
    const historyUrl = `/cgi-bin/search.cgi?history=1&user=${encodeURIComponent(username)}`;
    
    console.log("[history.js] Fetching history from:", historyUrl);

    fetch(historyUrl)
      .then(response => {
        console.log("[history.js] History response status:", response.status, response.statusText);
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        return response.json();
      })
      .then(data => {
        console.log("[history.js] History data received:", data);
        
        if (data.error) {
          showError(data.error);
          return;
        }

        if (!Array.isArray(data) || data.length === 0) {
          showNoHistory();
          return;
        }

        // Update count
        historyCountElement.textContent = `${data.length} recent searches`;

        // Build history HTML
        let historyHTML = "";
        data.forEach((item, index) => {
          historyHTML += `
            <div class="history-item" data-search-term="${item.search_term}" title="Click to search again">
              <span class="search-term">${item.search_term}</span>
              <span class="search-timestamp">${item.timestamp}</span>
            </div>
          `;
        });

        searchHistoryElement.innerHTML = historyHTML;

        // Add click handlers to history items
        const historyItems = searchHistoryElement.querySelectorAll('.history-item');
        historyItems.forEach(item => {
          item.addEventListener('click', function() {
            const searchTerm = this.getAttribute('data-search-term');
            console.log("[history.js] History item clicked:", searchTerm);
            
            // Redirect to main page with the search term
            window.location.href = `index.html?query=${encodeURIComponent(searchTerm)}`;
          });
        });
      })
      .catch(error => {
        console.error("[history.js] Error loading history:", error);
        showError(error.message);
      });
  }

  // Refresh button handler
  refreshButton.addEventListener("click", function() {
    console.log("[history.js] Refresh button clicked");
    loadSearchHistory();
  });

  // Load history on page load
  loadSearchHistory();

  console.log("[history.js] History page initialized for user:", currentUsername);
});