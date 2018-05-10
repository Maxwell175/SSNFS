<?php
if (check_auth_cookie($_COOKIE['SSNFS_auth']) == FALSE) {
    header("Location: /login.php");
    http_response_code(303);
    exit();
}
?>
