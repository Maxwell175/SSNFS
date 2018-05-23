<!--
 * SSNFS WebPanel v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 *
-->
<?php include_once("check-cookie.php"); ?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Dashboard - SSNFS</title>

    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css" integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.3.1/jquery.min.js" integrity="sha384-tsQFqpEReu7ZLhBV2VZlAu7zcOV+rXbYlF2cqB8txI/8aZajjp4Bqd+V6D5IgvKT" crossorigin="anonymous"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.js" integrity="sha384-PtTRqvDhycIBU6x1wwIqnbDo8adeWIWP3AHmnrvccafo35E7oIvW7HPXn2YimvWu" crossorigin="anonymous"></script>

    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.css" integrity="sha384-VK/ia2DWrvtO05YDcbWI8WE3WciOH0RhfPNuRJGSa3dpAs5szXWQuCnPNv/yzpO4" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js" integrity="sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49" crossorigin="anonymous"></script>

    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jqueryui-touch-punch/0.2.3/jquery.ui.touch-punch.min.js" integrity="sha384-MI/QivrbkVVJ89UOOdqJK/w6TLx0MllO/LsQi9KvvJFuRHGbYtsBvbGSM8JHKCS0" crossorigin="anonymous"></script>

    <style>
        body {
            padding-top: 20px;
            padding-bottom: 20px;
        }

        .page-title {
            position: relative;
            box-shadow: 0px 1px 10px rgba(0,0,0,.3);
            margin: 10px 0;
            background-color: #e2e2e2;
        }
        .breadcrumb {
            background-color: #e2e2e2;
            font-size: 1.3em;
            font-weight: bold;
        }

        .widget-col {
            padding-bottom: 250px;
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
            height: 215px;
            overflow-y: scroll;
            padding: 0 15px;
        }

        .moreinfo-icon {
            font-size: 1.5em;
            padding: 3px !important;
        }
    </style>
</head>
<body>
<div class="container">

    <?php include_once("navbar.php"); ?>

    <header class="page-title">
        <ol class="breadcrumb">
            <li>Dashboard</li>
        </ol>
    </header>

    <div class="row">
        <div class="col-md-6 widget-col" id="widget-col1">
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
                        </tr>
                        </thead>
                        <tbody>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                        </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
        <div class="col-md-6 widget-col" id="widget-col2">
            <div class="panel panel-default" id="Widget_PendApprovals">
                <div class="panel-heading">
                    <h2 class="panel-title">Pending Computer Approvals</h2>
                </div>
                <div class="panel-body">
                    <table class="table table-striped">
                        <thead>
                        <tr>
                            <th>User</th>
                            <th>Computer</th>
                            <th>View</th>
                        </tr>
                        </thead>
                        <tbody>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        <tr>
                            <td>Test User</td>
                            <td>Test Computer</td>
                            <td class="moreinfo-icon"><a href="#"><i class="fas fa-info-circle"></i></a></td>
                        </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </div>

</div> <!-- /container -->

<script type="text/javascript">
    // Make panels sortable
    $('.container .col-md-6').sortable({
        handle: '.panel-heading',
        cursor: 'grabbing',
        connectWith: '.container .col-md-6',
    });
</script>
</body>
</html>