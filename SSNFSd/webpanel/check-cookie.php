<?php
/*
 * SSNFS WebPanel v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 *
 */
if (check_auth_cookie($_COOKIE['SSNFS_auth']) == FALSE) {
    header("Set-Cookie: SSNFS_auth=; Secure; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    header("Location: /login.php");
    http_response_code(303);
    exit();
}
