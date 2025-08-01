// history.js - Search history and theme functionality with delete feature
document.addEventListener("DOMContentLoaded", function () {
  const searchHistoryElement = document.getElementById("searchHistory");
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
    searchHistoryElement.innerHTML = '';
  }

  function showError(message) {
    searchHistoryElement.innerHTML = `<div class="error-message">Error: ${message}</div>`;
  }

   function showNoHistory() {
    searchHistoryElement.innerHTML = '<div class="no-history-message">No search history found.</div>';
  }

  function showConfirmation(title, message, onConfirm) {
    confirmationTitle.textContent = title;
    confirmationMessage.textContent = message;
    confirmationDialog.style.display = 'flex';
    
    const newConfirmYes = confirmYes.cloneNode(true);
    const newConfirmNo = confirmNo.cloneNode(true);
    confirmYes.parentNode.replaceChild(newConfirmYes, confirmYes);
    confirmNo.parentNode.replaceChild(newConfirmNo, confirmNo);
    
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
        historyItem.style.transition = 'all 0.3s ease';
        historyItem.style.opacity = '0';
        historyItem.style.transform = 'translateX(-100px)';
        
        setTimeout(() => {
          historyItem.remove();
          
          const remainingItems = searchHistoryElement.querySelectorAll('.history-item').length;
          
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
      
      if (deleteBtn) {
        deleteBtn.disabled = false;
        deleteBtn.textContent = 'Delete';
      }
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

        searchHistoryElement.querySelectorAll('.history-content').forEach(content => {
          content.addEventListener('click', function() {
            const searchTerm = this.getAttribute('data-search-term');
            window.location.href = `index.html?query=${encodeURIComponent(searchTerm)}`;
          });
        });

        searchHistoryElement.querySelectorAll('.delete-btn').forEach(deleteBtn => {
          deleteBtn.addEventListener('click', function(e) {
            e.stopPropagation();
            const historyId = this.getAttribute('data-history-id');
            const historyItem = this.closest('.history-item');
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
    if (username !== 'guest') {
      fetch(`/cgi-bin/search.cgi?get_settings=1&user=${username}`)
        .then(res => res.ok ? res.json() : { theme: 'light' })
        .then(settings => {
          applyTheme(settings.theme);
          loadSearchHistory();
        })
        .catch(err => {
          console.error("Failed to load theme, defaulting to light.css", err);
          applyTheme('light');
          loadSearchHistory();
        });
    } else {
        loadSearchHistory();
    }
  }

  // (REMOVED) Event listeners for the refresh and clear buttons are gone.

  confirmationDialog.addEventListener('click', function(e) {
    if (e.target === confirmationDialog) {
      confirmationDialog.style.display = 'none';
    }
  });

  init();
});
