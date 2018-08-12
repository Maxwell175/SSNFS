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
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>SSNFS - Approve Computer</title>

    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css" integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous">

    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">

    <style>
        table, td {
            text-align: right;
            vertical-align: middle;
            padding: 5px;
            font-size: 0.97em
        }
        table {
            width: 100%;
        }
        .heading {
            font-weight: bold;
            font-size: 1em;
        }
        .fa-edit {
            color: #337ab7;
        }
        .btn-primary {
            font-weight: bold;
            padding: 10px 0;
            margin: 5px 0;
        }
    </style>
</head>
<body>
<script type="text/javascript">
    try {
        if (window.self === window.top) {
            alert("This page will not work if accessed directly. Please properly access it using the dashboard.")
        }
    } catch (e) {
        alert("It looks like this page was loaded in an iframe outside of the current domain.");
    }
</script>
<?php
if (isset($_POST['key']) && isset($_POST['CompName'])) {
    $approveResult = approve_pending($_POST['key'], $_POST['CompName']);
    if ($approveResult != 'OK'): ?>
        <h3 style="color:darkred"><?php echo $approveResult; ?></h3>
    <?php else: ?>
        <h3 style="color:darkgreen">This computer has been approved.</h3>
        <script type="text/javascript">
            window.top.setTimeout(function () {
                window.top.location.reload(true);
            }, 5000)
        </script>
    <?php endif; ?>
    </body>
    </html>
    <?php
    exit();
}
?>
<div id="container">
    <form action="approve.php" method="post" onsubmit="Approve.disabled = true; return true;">
        <input type="hidden" value="" name="key" id="Key"/>
        <table>
            <tr>
                <td class="heading">User Name:</td>
                <td id="UserName"></td>
            </tr>
            <tr>
                <td class="heading">Computer Name:</td>
                <td>
                    <input type="text" value="" name="CompName" id="txtCompName" size="11" style="display: none;"/>
                    <span id="CompName">TST</span> <a href="javascript:void(0)"id="CompNameEdit"><i class="fas fa-edit"></i></a>
                </td>
            </tr>
            <tr>
                <td class="heading">Submitted On:</td>
                <td id="SubmitTmStmp"></td>
            </tr>
            <tr>
                <td class="heading">Submitted By:</td>
                <td id="SubmitBy"></td>
            </tr>
        </table>
        <button id="btnApprove" type="submit" class="btn btn-primary btn-block" name="Approve"> Approve </button>
    </form>
</div>

<script type="text/javascript">
    var editCompName = document.getElementById('CompNameEdit');
    var lblCompName = document.getElementById('CompName');
    var txtCompName = document.getElementById('txtCompName');

    var txtKey = document.getElementById('Key');
    var lblUserName = document.getElementById('UserName');
    var lblSubmitTmStmp = document.getElementById('SubmitTmStmp');
    var lblSubmitBy = document.getElementById('SubmitBy');
    txtKey.value = window.parent.currApprovalInfo.pendingClientKey;
    lblUserName.innerHTML = window.parent.currApprovalInfo.userName;
    lblSubmitTmStmp.innerHTML = window.parent.currApprovalInfo.submitTmStmp.toLocaleString();
    lblSubmitBy.innerHTML = window.parent.currApprovalInfo.submitHost;
    lblCompName.innerHTML = window.parent.currApprovalInfo.clientName;
    txtCompName.value = window.parent.currApprovalInfo.clientName;

    var containerDiv = document.getElementById('container');

    function resizeFrame() {
        var bounds = containerDiv.getBoundingClientRect();

        if (window.parent.currApprovalInfo.iframe.height != bounds.height + 5)
        window.parent.currApprovalInfo.iframe.height = bounds.height + 5;
    }
    resizeFrame();
    setInterval(resizeFrame, 500);

    editCompName.onclick = function() {
        lblCompName.style.display = 'none';
        editCompName.style.display = 'none';
        txtCompName.style.display = '';
        resizeFrame();
    };
</script>
</body>
