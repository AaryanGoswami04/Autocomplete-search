document.addEventListener("DOMContentLoaded", function () {
    const usernameSpan = document.getElementById("username");
    const passwordSpan = document.getElementById("password");
    const passwordChangeForm = document.getElementById("passwordChangeForm");
    const newPasswordInput = document.getElementById("newPassword");
    const confirmPasswordInput = document.getElementById("confirmPassword");
    const messageDiv = document.getElementById("message");
    const themeStylesheet = document.getElementById('theme-stylesheet'); // Get the stylesheet link

    const currentUser = sessionStorage.getItem('currentUser');

    if (!currentUser) {
        window.location.href = 'login.html';
        return;
    }

    // --- NEW: Theme Loading Logic ---
    // This function applies the selected theme by changing the CSS file.
    function applyTheme(themeName) {
        themeStylesheet.href = `${themeName}.css`;
    }

    // Fetch the user's settings to apply the theme on page load.
    if (currentUser !== 'guest') {
        fetch(`/cgi-bin/search.cgi?get_settings=1&user=${encodeURIComponent(currentUser)}`)
          .then(res => res.ok ? res.json() : { theme: 'light' }) // Default to light on error
          .then(settings => {
            if (settings && settings.theme) {
                applyTheme(settings.theme);
            }
          })
          .catch(err => {
              console.error("Failed to load theme, defaulting to light.css", err);
              applyTheme('light');
          });
    }
    // --- End of new logic ---


    // Function to display messages
    function showMessage(message, type) {
        messageDiv.textContent = message;
        messageDiv.className = `visible ${type}`;
        setTimeout(() => {
            messageDiv.className = '';
        }, 4000);
    }

    // Function to fetch and display profile data
    function fetchProfileData() {
        fetch(`/cgi-bin/search.cgi?get_profile=1&user=${encodeURIComponent(currentUser)}`)
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(data => {
                if (data.username && data.password) {
                    usernameSpan.textContent = data.username;
                    passwordSpan.textContent = data.password;
                } else {
                    showMessage('Could not retrieve profile data.', 'error');
                }
            })
            .catch(error => {
                console.error('Error fetching profile data:', error);
                showMessage('An error occurred while fetching your data.', 'error');
            });
    }

    // Handle password change form submission
    passwordChangeForm.addEventListener("submit", function (e) {
        e.preventDefault();
        const newPassword = newPasswordInput.value;
        const confirmPassword = confirmPasswordInput.value;

        if (newPassword.length < 3) {
            showMessage('Password must be at least 3 characters long.', 'error');
            return;
        }

        if (newPassword !== confirmPassword) {
            showMessage('Passwords do not match.', 'error');
            return;
        }

        fetch(`/cgi-bin/search.cgi?update_password=1&user=${encodeURIComponent(currentUser)}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain;charset=UTF-8'
            },
            body: newPassword
        })
        .then(response => response.text())
        .then(text => {
            showMessage(text, 'success');
            passwordChangeForm.reset();
            // Refresh the displayed password
            fetchProfileData();
        })
        .catch(error => {
            console.error('Error updating password:', error);
            showMessage('An error occurred during password update.', 'error');
        });
    });

    // Initial data load
    fetchProfileData();
});