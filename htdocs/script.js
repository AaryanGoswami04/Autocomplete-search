// script.js
document.addEventListener("DOMContentLoaded", function () {
  const searchBox = document.getElementById("searchBox");
  const suggestions = document.getElementById("suggestions");
  const uploadInput = document.getElementById("wordFile");
  const searchHistoryContainer = document.getElementById("searchHistory");


  // Username handling - FIXED
  const urlParams = new URLSearchParams(window.location.search);
  const loggedInUser = urlParams.get("user");
  
  // Store username in sessionStorage to persist across page refreshes
  let currentUsername = loggedInUser || sessionStorage.getItem('currentUser') || "guest";
  
  // If we got a user from URL, store it in sessionStorage
  if (loggedInUser) {
    sessionStorage.setItem('currentUser', loggedInUser);
    // Clean URL after storing username
    const cleanUrl = window.location.pathname;
    window.history.replaceState({}, document.title, cleanUrl);
  }
  // Cache for search history to prevent unnecessary DOM updates
  let cachedHistoryItems = [];
  let lastHistoryUpdate = 0;

  // Search History Functions
  function formatTimestamp(timestamp) {
    try {
      const date = new Date(timestamp);
      if (isNaN(date.getTime())) {
        return "Unknown time";
      }
      
      const now = new Date();
      const diffMs = now - date;
      const diffMins = Math.floor(diffMs / 60000);
      const diffHours = Math.floor(diffMs / 3600000);
      const diffDays = Math.floor(diffMs / 86400000);

      if (diffMins < 1) return "Just now";
      if (diffMins < 60) return `${diffMins}m ago`;
      if (diffHours < 24) return `${diffHours}h ago`;
      if (diffDays < 7) return `${diffDays}d ago`;
      
      return date.toLocaleDateString();
    } catch (error) {
      console.error("[script.js] Error formatting timestamp:", error);
      return "Unknown time";
    }
  }

  // Compare two history arrays to see if they're different
  function historyHasChanged(newItems, oldItems) {
    if (newItems.length !== oldItems.length) {
      return true;
    }
    
    for (let i = 0; i < newItems.length; i++) {
      if (newItems[i].search_term !== oldItems[i].search_term || 
          newItems[i].timestamp !== oldItems[i].timestamp) {
        return true;
      }
    }
    
    return false;
  }

  // Update only the timestamps without rebuilding DOM
  function updateTimestampsOnly() {
    const timestampElements = searchHistoryContainer.querySelectorAll('.search-timestamp');
    timestampElements.forEach((element, index) => {
      if (cachedHistoryItems[index]) {
        const newTimestamp = formatTimestamp(cachedHistoryItems[index].timestamp);
        if (element.textContent !== newTimestamp) {
          element.textContent = newTimestamp;
        }
      }
    });
  }

  function loadSearchHistory(forceUpdate = false) {
    const username = currentUsername || "guest";
    const historyUrl = `/cgi-bin/search.cgi?history=1&user=${encodeURIComponent(username)}`;
    
    console.log("[script.js] Loading search history for user:", username);

    // Don't show loading message on periodic updates
    if (forceUpdate) {
      searchHistoryContainer.innerHTML = '<div class="loading-message">Loading search history...</div>';
    }

    fetch(historyUrl)
      .then(response => {
        console.log("[script.js] History response status:", response.status, response.statusText);
        
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        return response.text();
      })
      .then(data => {
        console.log("[script.js] History response data length:", data.length);
        displaySearchHistory(data, forceUpdate);
      })
      .catch(error => {
        console.error("[script.js] Error fetching search history:", error);
        if (forceUpdate) {
          searchHistoryContainer.innerHTML = `
            <div class="error-message">
              Failed to load search history: ${error.message}
              <br><small>Check browser console for details</small>
            </div>
          `;
        }
      });
  }

  let historyPoll;
  function startHistoryPolling() {
    // Poll every 10 seconds instead of 3 (less aggressive)
    historyPoll = setInterval(() => {
      loadSearchHistory(false); 
    }, 10000);
  }

  function stopHistoryPolling() {
    if (historyPoll) {
      clearInterval(historyPoll);
      historyPoll = null;
    }
  }

  // Stop polling when page is not visible to save resources
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
      stopHistoryPolling();
    } else {
      startHistoryPolling();
      // Refresh once when page becomes visible again
      setTimeout(() => loadSearchHistory(false), 500);
    }
  });

  window.addEventListener('beforeunload', stopHistoryPolling);

  function displaySearchHistory(data, forceUpdate = false) {
    console.log("[script.js] Displaying search history, force update:", forceUpdate);
    
    if (!data || data.trim() === "") {
      if (cachedHistoryItems.length > 0 || forceUpdate) {
        searchHistoryContainer.innerHTML = '<div class="no-history-message">No search history available</div>';
        cachedHistoryItems = [];
      }
      return;
    }

    try {
      let historyItems = [];
      
      // Check if response contains error
      if (data.includes('"error"')) {
        const errorObj = JSON.parse(data);
        searchHistoryContainer.innerHTML = `<div class="error-message">Database error: ${errorObj.error}</div>`;
        return;
      }
      
      // Parse JSON response
      if (data.trim().startsWith('[')) {
        historyItems = JSON.parse(data);
        console.log("[script.js] Parsed JSON history items:", historyItems);
      } else {
        console.warn("[script.js] Unexpected response format:", data);
        if (forceUpdate) {
          searchHistoryContainer.innerHTML = '<div class="error-message">Unexpected response format from server</div>';
        }
        return;
      }

      if (!Array.isArray(historyItems)) {
        console.error("[script.js] History items is not an array:", historyItems);
        if (forceUpdate) {
          searchHistoryContainer.innerHTML = '<div class="error-message">Invalid data format received</div>';
        }
        return;
      }

      // Check if history has actually changed
      if (!forceUpdate && !historyHasChanged(historyItems, cachedHistoryItems)) {
        // Only update timestamps for relative time display
        updateTimestampsOnly();
        return;
      }

      // Update cache
      cachedHistoryItems = [...historyItems];
      lastHistoryUpdate = Date.now();

      if (historyItems.length === 0) {
        searchHistoryContainer.innerHTML = '<div class="no-history-message">No search history yet. Start searching to see your history here!</div>';
        return;
      }

      // Only rebuild DOM when necessary
      console.log("[script.js] Rebuilding history DOM with", historyItems.length, "items");
      
      // Use DocumentFragment for better performance
      const fragment = document.createDocumentFragment();

      historyItems.forEach((item, index) => {
        console.log("[script.js] Processing history item", index, ":", item);
        
        const historyItem = document.createElement('div');
        historyItem.className = 'history-item';
        
        const searchTerm = document.createElement('span');
        searchTerm.className = 'search-term';
        searchTerm.textContent = item.search_term || 'Unknown search';
        
        const timestamp = document.createElement('span');
        timestamp.className = 'search-timestamp';
        timestamp.textContent = formatTimestamp(item.timestamp);
        
        // Make history items clickable
        historyItem.addEventListener('click', (e) => {
          console.log("[script.js] History item clicked:", item.search_term);
          searchBox.value = item.search_term;
          searchBox.focus();
          searchBox.dispatchEvent(new Event("input"));
        });
        
        historyItem.style.cursor = 'pointer';
        historyItem.title = `Click to search for "${item.search_term}"`;
        
        historyItem.appendChild(searchTerm);
        historyItem.appendChild(timestamp);
        fragment.appendChild(historyItem);
      });

      // Replace content in one operation to minimize reflow
      searchHistoryContainer.innerHTML = "";
      searchHistoryContainer.appendChild(fragment);

      console.log("[script.js] Successfully displayed", historyItems.length, "history items");

    } catch (error) {
      console.error("[script.js] Error parsing/displaying search history:", error);
      console.error("[script.js] Raw data was:", data);
      if (forceUpdate) {
        searchHistoryContainer.innerHTML = `
          <div class="error-message">
            Error displaying search history: ${error.message}
            <br><small>Raw response: ${data.substring(0, 100)}${data.length > 100 ? '...' : ''}</small>
          </div>
        `;
      }
    }
  }

  function refreshSearchHistory() {
    console.log("[script.js] Refreshing search history...");
    // Immediate update after user action
    setTimeout(() => {
      loadSearchHistory(false); // Don't show loading message
    }, 500);
  }

  // Initialize search history on page load
  console.log("[script.js] Loading initial search history...");
  loadSearchHistory(true); // Force update on initial load
  
  // Start polling after initial load
  startHistoryPolling();

  // File Upload Handler
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
        const uploadUrl = "/cgi-bin/search.cgi";
        
        console.log("[script.js] POST to:", uploadUrl);

        fetch(uploadUrl, {
          method: "POST",
          body: fileContent,
          headers: { "Content-Type": "text/plain;charset=UTF-8" }
        })
          .then(response => {
            console.log("[script.js] Upload response status:", response.status, response.statusText);
            return response.text();
          })
          .then(text => {
            console.log("[script.js] Upload response:", text);
          })
          .catch(err => {
            console.error("[script.js] Upload error:", err);
          });
      };

      reader.onerror = (e) => console.error("[script.js] FileReader error:", e);
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
    
    console.log("[script.js] Search request:", searchUrl);

    fetch(searchUrl)
      .then(response => {
        console.log("[script.js] Search response status:", response.status, response.statusText);
        return response.text();
      })
      .then(data => {
        suggestions.innerHTML = "";

        const lines = data.split("\n").filter(line => line.startsWith(" - "));
        console.log("[script.js] Found", lines.length, "suggestions for:", query);

        lines.forEach(line => {
          const item = document.createElement("li");
          item.textContent = line.replace(" - ", "");

          item.addEventListener("click", () => {
            const chosen = item.textContent;
            console.log("[script.js] Suggestion clicked:", chosen);
            
            searchBox.value = chosen;
            suggestions.innerHTML = "";

            // Log the click
            const clickUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(chosen)}&user=${encodeURIComponent(username)}&log=1`;
            console.log("[script.js] Logging click:", clickUrl);
            
            fetch(clickUrl)
              .then(r => r.text())
              .then(t => {
                console.log("[script.js] Click logged successfully, response:", t);
                refreshSearchHistory();
              })
              .catch(err => {
                console.error("[script.js] Click log error:", err);
              });
          });

          suggestions.appendChild(item);
        });
      })
      .catch(error => {
        console.error("[script.js] Search error:", error);
      });
  });

  // Enter Key Handler
  searchBox.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      const term = searchBox.value.trim();
      if (!term) return;

      console.log("[script.js] Enter pressed with term:", term);

      const username = currentUsername || "guest";
      const enterUrl = `/cgi-bin/search.cgi?query=${encodeURIComponent(term)}&user=${encodeURIComponent(username)}&log=1`;
      
      console.log("[script.js] Logging enter search:", enterUrl);
      
      fetch(enterUrl)
        .then(r => r.text())
        .then(t => {
          console.log("[script.js] Enter search logged successfully, response:", t);
          refreshSearchHistory();
        })
        .catch(err => {
          console.error("[script.js] Enter log error:", err);
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

  // Debug function to show current username
  window.debugUser = function() {
    console.log("Current username:", currentUsername);
    console.log("SessionStorage user:", sessionStorage.getItem('currentUser'));
    console.log("Cached history items:", cachedHistoryItems.length);
    console.log("Last history update:", new Date(lastHistoryUpdate));
  };

  console.log("[script.js] Initialization complete");
});