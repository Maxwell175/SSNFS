<?php
if (check_auth_cookie($_COOKIE['SSNFS_auth']) == TRUE) {
    header("Location: /");
    http_response_code(303);
    exit();
}

if (isset($_POST['email'])) {
    exit();
}
?>
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Login - SSNFS</title>

        <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css" integrity="sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB" crossorigin="anonymous">

        <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css" integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous">

        <script type="text/javascript" src="crypto-js/build/rollups/sha512.js"></script>

        <style>
            html, body{height:100%; margin:0;padding:0}

            .container-fluid{
                height:100%;
                display:table;
                width: 100%;
                padding: 0;
            }

            .row-fluid {height: 100%; display:table-cell; vertical-align: middle;}

            .centering {
                float:none;
                margin:0 auto;
            }

            .card-body {
                padding: 1em 2em;
            }
        </style>
    </head>
    <body>
        <div class="container-fluid">
            <div class="row-fluid">
                <div class="col-md-5 centering">
                    <div class="card bg-light">
                        <article class="card-body">
                            <form action="login.php" method="post">
                                <h2><b>SSNFS</b></h2>
                                <h4>Login</h4>
                                <div class="form-group input-group">
                                    <div class="input-group-prepend">
                                        <span class="input-group-text"> <i class="fa fa-envelope"></i> </span>
                                    </div>
                                    <input name="email" autofocus="autofocus" class="form-control" placeholder="Email address" type="email">
                                </div> <!-- form-group// -->
                                <div class="form-group input-group">
                                    <div class="input-group-prepend">
                                        <span class="input-group-text"> <i class="fa fa-lock"></i> </span>
                                    </div>
                                    <input id="txtPassword" class="form-control" placeholder="Password" type="password" name="">
                                    <input id="txtPasswordHash" type="hidden" name="passwordHash">
                                </div> <!-- form-group// -->
                                <div class="form-group">
                                    <button id="btnLogin" type="submit" class="btn btn-primary btn-block"> Login </button>
                                </div> <!-- form-group// -->
                            </form>
                        </article>
                    </div> <!-- card.// -->
                </div>
            </div>
        </div>
        <script type="text/javascript">
            var txtPassword = document.getElementById("txtPassword");
            var txtPasswordHash = document.getElementById("txtPasswordHash");
            var btnLogin = document.getElementById("frmLogin");
            
            txtPassword.onkeyup = function() {
                txtPasswordHash.value = CryptoJS.SHA512(txtPassword.value);
            };

            btnLogin.onclick = function () {
                txtPassword.disabled = true;
                txtPassword.value = '0'.repeat(txtPassword.value.length);
            };
        </script>
    </body>
</html>
