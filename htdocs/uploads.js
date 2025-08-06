document.addEventListener("DOMContentLoaded", function () {
  // Get necessary elements from the DOM
  const uploadListElement = document.getElementById("uploadList");
  const themeStylesheet = document.getElementById('theme-stylesheet');
  // Retrieve the current user from session storage, defaulting to 'guest'
  const username = sessionStorage.getItem('currentUser') || "guest";

  /**
   * Applies the selected theme by changing the stylesheet href.
   * @param {string} themeName - The name of the theme to apply (e.g., 'light' or 'dark').
   */
  function applyTheme(themeName) {
    if (themeName) {
      themeStylesheet.href = `${themeName}.css`;
      // Store the theme choice for persistence across pages
      localStorage.setItem('theme', themeName);
    }
  }
  //Displays a loading message in the upload list.

  function showLoading() {
    uploadListElement.innerHTML = '<li class="loading-message">Loading your uploads...</li>';
  }

 //Displays a message indicating no files have been uploaded.
  
  function showNoUploads() {
    uploadListElement.innerHTML = '<li class="empty-message">You have not uploaded any files yet.</li>';
  }

  // Fetches the list of uploaded files from the server and populates the list.
  function loadUploads() {
    showLoading();
    
    // Construct the URL to fetch user-specific uploads
    const uploadsUrl = `/cgi-bin/search.cgi?uploads=1&user=${encodeURIComponent(username)}`;
    
    fetch(uploadsUrl)
      .then(response => response.json())
      .then(data => {
        if (!Array.isArray(data) || data.length === 0) {
          showNoUploads();
          return;
        }

        // Build the HTML for the list of uploaded files
        let uploadsHTML = "";
        data.forEach(file => {
          uploadsHTML += `
            <li class="upload-item">
              <span class="upload-name">${file.filename}</span>
              <span class="upload-time">${file.upload_time}</span>
            </li>
          `;
        });

        uploadListElement.innerHTML = uploadsHTML;
      });
  }
  
  //Initializes the page by first fetching the theme, applying it and then loading the content. This prevents content from loadingin the wrong theme.
  function init() {
    if (username !== 'guest') {
      // 1. Fetch the user's saved theme setting first.
      fetch(`/cgi-bin/search.cgi?get_settings=1&user=${username}`)
        .then(res => res.json())
        .then(settings => {
          // 2. Apply the fetched theme.
          applyTheme(settings.theme);
          // 3. THEN, load the page content.
          loadUploads();
        });
    } else {
        // For guests, just load uploads with the default theme.
        applyTheme('light');
        loadUploads();
    }
  }
  init();
});