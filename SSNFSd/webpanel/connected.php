<!--
 * SSNFS WebPanel v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 *
-->
<?php include_once("check-cookie.php"); if (http_response_code() == 303) exit; ?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Connected Computers - SSNFS</title>

    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css" integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.3.1/jquery.min.js" integrity="sha384-tsQFqpEReu7ZLhBV2VZlAu7zcOV+rXbYlF2cqB8txI/8aZajjp4Bqd+V6D5IgvKT" crossorigin="anonymous"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js" integrity="sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49" crossorigin="anonymous"></script>

    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">

    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>

    <style>
        body {
            padding-top: 20px;
            padding-bottom: 20px;
        }

        .panel-default>.panel-heading {
            color: #fff;
            background-color: #1d233a;
            cursor: move;
        }

        .panel-title {
            font-size: 1em;
            font-weight: bold;
        }

        .panel-body {
            min-height: 215px;
            padding: 0 15px;
        }

        .no-data {
            color: darkgray;
            text-align: center;
        }
    </style>
</head>
<body>
<div class="container">

    <?php include_once("navbar.php"); ?>

    <div class="panel panel-default" id="Widget_ConnectedClients">
        <div class="panel-heading">
            <h2 class="panel-title">Connected Computers</h2>
        </div>
        <div class="panel-body">
            <table class="table table-striped">
                <thead>
                <tr>
                    <th>User</th>
                    <th>Computer</th>
                    <th>Share</th>
                </tr>
                </thead>
                <tbody>
                <?php foreach(get_connected() as $client): ?>
                    <tr>
                        <td><a href="users.php?key=<?php echo $client["userKey"]; ?>"><?php echo $client["userName"]; ?></a></td>
                        <td><a href="computers.php?key=<?php echo $client["clientKey"]; ?>"><?php echo $client["clientName"]; ?></a></td>
                        <td><a href="shares.php?key=<?php echo $client["shareKey"]; ?>"><?php echo $client["shareName"]; ?></a></td>
                    </tr>
                <?php endforeach; ?>
                </tbody>
            </table>
            <?php if (!isset($client)): ?>
                <div class="no-data">There are no active connections.</div>
            <?php endif; ?>
        </div>
    </div>

</div> <!-- /container -->
</body>
</html>