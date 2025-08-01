// history.js - Search history and theme functionality with delete feature
document.addEventListener("DOMContentLoaded", function () {
  const searchHistoryElement = document.getElementById("searchHistory");
  const historyCountElement = document.getElementById("historyCount");
  const refreshButton = document.getElementById("refreshHistory");
  const clearAllButton = document.getElementById("clearAllHistory");
  const themeStylesheet = document.getElementById('theme-stylesheet');
  const confirmationDialog = document.getElementById('confirmationDialog');
  const confirmationTitle = document.getElementById('confirmationTitle');
  const confirmationMessage = document.getElementById('confirmationMessage');
  const confirmYes = document.getElementById('confirmYes');
  const confirmNo = document.getElementById('confirmNo');

  const username = sessionStorage.getItem('currentUser') || "guest";
  let currentHistoryData = [];

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

  function showConfirmation(title, message, onConfirm) {
    confirmationTitle.textContent = title;
    confirmationMessage.textContent = message;
    confirmationDialog.style.display = 'flex';
    
    // Remove any existing event listeners
    const newConfirmYes = confirmYes.cloneNode(true);
    const newConfirmNo = confirmNo.cloneNode(true);
    confirmYes.parentNode.replaceChild(newConfirmYes, confirmYes);
    confirmNo.parentNode.replaceChild(newConfirmNo, confirmNo);
    
    // Add new event listeners
    newConfirmYes.addEventListener('click', () => {
      confirmationDialog.style.display = 'none';
      onConfirm();
    });
    
    newConfirmNo.addEventListener('click', () => {
      confirmationDialog.style.display = 'none';
    });
  }

  function deleteHistoryItem(historyId, historyItem) {
    const deleteUrl = `/cgi-bin/search.cgi?delete_history=1&history_id=${historyId}&user=${encodeURIComponent(username)}`;
    
    // Disable the delete button to prevent multiple clicks
    const deleteBtn = historyItem.querySelector('.delete-btn');
    if (deleteBtn) {
      deleteBtn.disabled = true;
      deleteBtn.textContent = 'Deleting...';
    }
    
    fetch(deleteUrl, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      }
    })
    .then(response => {
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    })
    .then(data => {
      if (data.success) {
        // Remove the item from the DOM with animation
        historyItem.style.transition = 'all 0.3s ease';
        historyItem.style.opacity = '0';
        historyItem.style.transform = 'translateX(-100px)';
        
        setTimeout(() => {
          historyItem.remove();
          
          // Update the count
          const remainingItems = searchHistoryElement.querySelectorAll('.history-item').length;
          historyCountElement.textContent = `${remainingItems} recent searches`;
          
          // Show "no history" message if all items are deleted
          if (remainingItems === 0) {
            showNoHistory();
          }
        }, 300);
      } else {
        throw new Error(data.error || 'Failed to delete history item');
      }
    })
    .catch(error => {
      console.error("Error deleting history item:", error);
      alert(`Failed to delete history item: ${error.message}`);
      
      // Re-enable the delete button
      if (deleteBtn) {
        deleteBtn.disabled = false;
        deleteBtn.textContent = 'Delete';
      }
    });
  }

  function clearAllHistory() {
    if (currentHistoryData.length === 0) {
      alert('No history to clear.');
      return;
    }

    // Delete all history items one by one
    const deletePromises = currentHistoryData.map(item => {
      const deleteUrl = `/cgi-bin/search.cgi?delete_history=1&history_id=${item.id}&user=${encodeURIComponent(username)}`;
      return fetch(deleteUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        }
      });
    });

    Promise.all(deletePromises)
      .then(responses => {
        // Check if all requests were successful
        const allSuccessful = responses.every(response => response.ok);
        if (allSuccessful) {
          showNoHistory();
          currentHistoryData = [];
        } else {
          throw new Error('Some items could not be deleted');
        }
      })
      .catch(error => {
        console.error("Error clearing all history:", error);
        alert(`Failed to clear all history: ${error.message}`);
        // Reload the history to show the current state
        loadSearchHistory();
      });
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
          currentHistoryData = [];
          return;
        }

        currentHistoryData = data;
        historyCountElement.textContent = `${data.length} recent searches`;

        let historyHTML = "";
        data.forEach(item => {
          historyHTML += `
            <div class="history-item" data-history-id="${item.id}">
              <div class="history-content" data-search-term="${item.search_term}" title="Click to search again">
                <span class="search-term">${item.search_term}</span>
                <span class="search-timestamp">${item.timestamp}</span>
              </div>
              <button class="delete-btn" data-history-id="${item.id}" title="Delete this search">Delete</button>
            </div>
          `;
        });

        searchHistoryElement.innerHTML = historyHTML;

        // Add click handlers for search terms
        searchHistoryElement.querySelectorAll('.history-content').forEach(content => {
          content.addEventListener('click', function() {
            const searchTerm = this.getAttribute('data-search-term');
            window.location.href = `index.html?query=${encodeURIComponent(searchTerm)}`;
          });
        });

        // Add click handlers for delete buttons
        searchHistoryElement.querySelectorAll('.delete-btn').forEach(deleteBtn => {
          deleteBtn.addEventListener('click', function(e) {
            e.stopPropagation(); // Prevent triggering the search term click
            const historyId = this.getAttribute('data-history-id');
            const historyItem = this.closest('.history-item');
            
            // Delete immediately without confirmation
            deleteHistoryItem(historyId, historyItem);
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

  // Event listeners
  refreshButton.addEventListener("click", loadSearchHistory);
  
  clearAllButton.addEventListener("click", function() {
    showConfirmation(
      'Clear All History',
      'Are you sure you want to delete all your search history? This action cannot be undone.',
      clearAllHistory
    );
  });

  // Close confirmation dialog when clicking outside
  confirmationDialog.addEventListener('click', function(e) {
    if (e.target === confirmationDialog) {
      confirmationDialog.style.display = 'none';
    }
  });

  // Initialize the page
  init();
});