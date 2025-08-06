document.addEventListener("DOMContentLoaded", function () {
    const usernameSpan = document.getElementById("username");
    const passwordSpan = document.getElementById("password");
    const passwordChangeForm = document.getElementById("passwordChangeForm");
    const newPasswordInput = document.getElementById("newPassword");
    const confirmPasswordInput = document.getElementById("confirmPassword");
    const messageDiv = document.getElementById("message");
    const themeStylesheet = document.getElementById('theme-stylesheet');
    const currentUser = sessionStorage.getItem('currentUser');
    if (!currentUser) {
        window.location.href = 'login.html';
        return;
    }
    function applyTheme(themeName) {
        themeStylesheet.href = `${themeName}.css`;
    }

   
    if (currentUser !== 'guest') {
        fetch(`/cgi-bin/search.cgi?get_settings=1&user=${encodeURIComponent(currentUser)}`)
          .then(res => res.json())
          .then(settings => {
            if (settings && settings.theme) {
                applyTheme(settings.theme);
            }
          });
    }

    // Function to display messages
    function showMessage(message, type) {
        messageDiv.textContent = message;
        messageDiv.className = `visible ${type}`;
        setTimeout(() => {
            messageDiv.className = '';
        }, 4000);
    }

    function fetchProfileData() {
        fetch(`/cgi-bin/search.cgi?get_profile=1&user=${encodeURIComponent(currentUser)}`)
            .then(response => response.json())
            .then(data => {
                if (data.username && data.password) {
                    usernameSpan.textContent = data.username;
                    passwordSpan.textContent = data.password;
                }
            });
    }

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
            fetchProfileData();
        });
    });
    fetchProfileData();
});