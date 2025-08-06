document.addEventListener("DOMContentLoaded", function () {
    const savedList = document.getElementById("saved-list");
    const loadingMessage = document.getElementById("loading-message");
    const emptyMessage = document.getElementById("empty-message");
    const errorMessage = document.getElementById("error-message");
    const username = sessionStorage.getItem('currentUser') || 'guest';
    // --- Apply Theme on Load ---
    function applyTheme() {
        if (username !== 'guest') {
            fetch(`/cgi-bin/search.cgi?get_settings=1&user=${username}`)
              .then(res => res.json())
              .then(settings => {
                document.getElementById('theme-stylesheet').href = `${settings.theme}.css`;
              });
        }
    }
    
    function fetchSavedSearches() {
        if (username === 'guest') {
            loadingMessage.style.display = 'none';
            errorMessage.textContent = 'You must be logged in to view saved searches.';
            errorMessage.style.display = 'block';
            return;
        }

        const url = `/cgi-bin/search.cgi?get_saved=1&user=${encodeURIComponent(username)}`;

        fetch(url)
            .then(response => response.json())
            .then(data => {
                loadingMessage.style.display = 'none';
                savedList.innerHTML = ''; // Clear previous items

                if (data.length === 0) {
                    emptyMessage.style.display = 'block';
                } else {
                    emptyMessage.style.display = 'none';
                    data.forEach(item => {
                        const li = document.createElement("li");
                        li.className = 'saved-item';
                        li.dataset.id = item.id;

                        const termDiv = document.createElement('div');
                        const termSpan = document.createElement('span');
                        termSpan.className = 'saved-term';
                        termSpan.textContent = item.search_term;
                        const timeSpan = document.createElement('span');
                        timeSpan.className = 'saved-timestamp';
                        timeSpan.textContent = `Saved on: ${new Date(item.timestamp).toLocaleString()}`;
                        termDiv.appendChild(termSpan);
                        termDiv.appendChild(document.createElement('br'));
                        termDiv.appendChild(timeSpan);

                        const actionsDiv = document.createElement('div');
                        actionsDiv.className = 'history-actions'; // Re-use style

                        const deleteBtn = document.createElement('button');
                        deleteBtn.textContent = 'Delete';
                        deleteBtn.className = 'action-btn'; // Re-use style
                        deleteBtn.onclick = () => deleteSavedSearch(item.id);

                        const searchBtn = document.createElement('button');
                        searchBtn.textContent = 'Search Again';
                        searchBtn.className = 'action-btn';
                        searchBtn.style.marginLeft = '10px';
                        searchBtn.onclick = () => window.location.href = `index.html?query=${encodeURIComponent(item.search_term)}`;

                        actionsDiv.appendChild(deleteBtn);
                        actionsDiv.appendChild(searchBtn);
                        
                        li.appendChild(termDiv);
                        li.appendChild(actionsDiv);
                        savedList.appendChild(li);
                    });
                }
            });
    }

    function deleteSavedSearch(savedId) {
        const url = `/cgi-bin/search.cgi?delete_saved=1&user=${encodeURIComponent(username)}&saved_id=${savedId}`;

        fetch(url, { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    const itemToRemove = document.querySelector(`.saved-item[data-id='${savedId}']`);
                    if (itemToRemove) itemToRemove.remove();
                    if (savedList.children.length === 0) emptyMessage.style.display = 'block';
                }
            });
    }
    applyTheme();
    fetchSavedSearches();
});