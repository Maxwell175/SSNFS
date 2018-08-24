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
    <title>Dashboard - SSNFS</title>

    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css" integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.3.1/jquery.min.js" integrity="sha384-tsQFqpEReu7ZLhBV2VZlAu7zcOV+rXbYlF2cqB8txI/8aZajjp4Bqd+V6D5IgvKT" crossorigin="anonymous"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.js" integrity="sha384-PtTRqvDhycIBU6x1wwIqnbDo8adeWIWP3AHmnrvccafo35E7oIvW7HPXn2YimvWu" crossorigin="anonymous"></script>

    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.css" integrity="sha384-VK/ia2DWrvtO05YDcbWI8WE3WciOH0RhfPNuRJGSa3dpAs5szXWQuCnPNv/yzpO4" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js" integrity="sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49" crossorigin="anonymous"></script>

    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/jqueryui-touch-punch/0.2.3/jquery.ui.touch-punch.min.js" integrity="sha384-MI/QivrbkVVJ89UOOdqJK/w6TLx0MllO/LsQi9KvvJFuRHGbYtsBvbGSM8JHKCS0" crossorigin="anonymous"></script>

    <script src="SpeechBubbleJS/SpeechBubble.js"></script>
    <link rel="stylesheet" href="SpeechBubbleJS/SpeechBubble.css">

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

        @media (min-width: 992px) {
            .widget-col {
                padding-bottom: 20vh;
            }
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

        .no-data {
            color: darkgray;
            text-align: center;
        }

        .speech-bubble-main {
            z-index: 1001;
            padding: 17px 15px 12px 15px
        }
        .closeBtn {
            position: absolute;
            top: 3px;
            right: 3px;
            font-size: 0.7em;
            color: white;
            background-color: darkred;
            padding: 5px 5px 4px 5px;
            border-radius: 4px;
            cursor: pointer;
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
                            <th>Share</th>
                        </tr>
                        </thead>
                        <tbody>
                        <?php foreach(get_connected() as $client): ?>
                            <tr>
                                <td><?php echo $client["userName"]; ?></td>
                                <td><?php echo $client["clientName"]; ?></td>
                                <td><?php echo $client["shareName"]; ?></td>
                            </tr>
                        <?php endforeach; ?>
                        </tbody>
                    </table>
                    <?php if (!isset($client)): ?>
                        <div class="no-data">There are no active connections.</div>
                    <?php endif; ?>
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
                        <?php foreach(get_pending() as $pendClient): ?>
                            <tr>
                                <td><?php echo $pendClient["userName"]; ?></td>
                                <td><?php echo $pendClient["clientName"]; ?></td>
                                <td class="moreinfo-icon"><a href="javascript:void(0)" onclick='window.showCompApprove(this, <?php echo json_encode($pendClient, JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP | JSON_FORCE_OBJECT); ?>)'>
                                        <i class="fas fa-info-circle"></i></a>
                                </td>
                            </tr>
                        <?php endforeach; ?>
                        </tbody>
                    </table>
                    <?php if (!isset($pendClient)): ?>
                        <div class="no-data">There are no pending computer approvals.</div>
                    <?php endif; ?>
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
        start: function() {
            if (window.currApprovalInfo && window.currApprovalInfo.iframe)
                window.currApprovalInfo.iframe.style.pointerEvents = 'none';
        },
        stop: function() {
            if (window.currApprovalInfo && window.currApprovalInfo.iframe)
                window.currApprovalInfo.iframe.style.pointerEvents = '';
        }
    });

    window.currApprovalInfo = {};
    window.showCompApprove = function(caller, data) {
        OpenSpeechBubbles.forEach(function(bubble) { bubble.removeBubble(); });
        var approveFrame = document.createElement("iframe");
        approveFrame.src = "approve.php";
        approveFrame.style.border = '0';
        approveFrame.width = '350';
        approveFrame.height = '168';
        window.currApprovalInfo = data;
        window.currApprovalInfo.submitTmStmp = new Date(window.currApprovalInfo.submitTmStmp*1000);
        window.currApprovalInfo.iframe = approveFrame;
        window.currApprovalInfo.approveWindow = SpeechBubble(caller, approveFrame);
        var closeBtn = document.createElement('i');
        closeBtn.className = 'fas fa-times closeBtn';
        window.currApprovalInfo.approveWindow.appendChild(closeBtn);
        document.documentElement.onclick = null;

        setTimeout(function () {
            document.documentElement.onclick = function (ev) {
                if (ev.target != window.currApprovalInfo.approveWindow) {
                    OpenSpeechBubbles.forEach(function(bubble) { bubble.removeBubble(); });
                    document.documentElement.onclick = null;
                }
            }
        }, 1);

    }
</script>
</body>
</html>