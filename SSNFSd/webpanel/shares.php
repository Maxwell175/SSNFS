<!--
 * SSNFS WebPanel v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 *
-->
<?php include_once("check-cookie.php"); if (http_response_code() == 303) exit;

if (isset($_POST["shareKey"]) && is_numeric($_POST["shareKey"]) && isset($_POST["shareName"]) && isset($_POST["localPath"]) && isset($_POST["shareUsers"])) {
    $shareUsers = json_decode($_POST["shareUsers"], true);
    $result = save_share((int)$_POST["shareKey"], $_POST["shareName"], $_POST["localPath"], $shareUsers );
    if ($result == "OK") {
        header("Location: shares.php?success=success");
        http_response_code(303);
    } else {
        header("Location: shares.php?error=" . rawurlencode($result));
        http_response_code(303);
    }
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Shares - SSNFS</title>

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

        .panel-default>.panel-heading, .panel-heading {
            color: #fff;
            background-color: #1d233a;
            position: relative;
            height: 45px;
        }

        .panel-title {
            font-size: 1.5em;
            font-weight: bold;
        }

        .panel-body {
            min-height: 215px;
            padding: 0;
        }
        form>.panel-body {
            width: 100%;
            padding: 5px 0 0;
            min-height: auto;
        }

        .form-control {
            height: 24px;
            padding: 0 6px;
            margin-bottom: 5px;
        }

        .form-group {
            padding: 10px 20px;
            border-bottom: 1px solid #E0E0E0;
            margin: 0 -15px;
        }

        .form-group:last-child {
            border-bottom: none;
        }

        .control-label {
            text-align: right;
        }

        .no-data {
            color: darkgray;
            text-align: center;
        }

        .editBtn {
            font-size: 1.5em;
            padding-left: 0.18em;
            color: #337ab7;
        }

        .saveBtn {
            font-size: 1.5em;
            font-weight: 300;
            margin: 0 auto 10px auto;
            min-width: 7em;
            display: block;
        }

        .addBtn {
            position: absolute;
            top: 5px;
            bottom: 5px;
            right: 5px;
        }

        #shareUsers {
            width: 100%;
            height: 300px;
            padding: 10px;
            background-color: #fffee9;
            border-radius: 20px;
            overflow-y: scroll;
        }

        .shareUser {
            width: 100%;
            padding: 15px;
            border: black 3px solid;
            background-color: #fff;
            border-radius: 10px;
        }

        .btnDelUser {
            font-size: 1.5em;
            display: block;
            position: absolute;
            right: 0;
            color: darkred;
            cursor: pointer;
        }

        .newUser {
            margin-top: 30px;
        }

        .clearfix:before,
        .clearfix:after {
            content: " ";
            display: table;
        }
        .clearfix:after {
            clear: both;
        }
        .clearfix {
            *zoom: 1;
        }
    </style>
</head>
<body>
    <div class="container">

        <?php include_once("navbar.php"); ?>

        <?php if (check_perm("shares")): ?>
            <?php if (isset($_GET["success"])): ?>
            <div class="alert alert-success" role="alert">
                The share has been saved successfully.
            </div>
            <?php endif;
            if (isset($_GET["error"])): ?>
            <div class="alert alert-danger" role="alert">
                <?php echo htmlentities($_GET["error"]); ?>
            </div>
            <?php endif; ?>

        <div class="panel panel-default" id="Widget_Shares">
            <?php if ((isset($_GET["key"]) && $_GET["key"] != "" && ctype_digit($_GET["key"])) || isset($_GET["new"])): ?>
                <?php if (isset($_GET["new"]))
                    $shareResults = array(array(
                        "shareKey" => -1,
                        "shareName" => "",
                        "localPath" => "",
                        "updtTmStmp" => 0,
                        "updtUser" => "",
                        "users" => array()
                    ));
                else
                    $shareResults = get_shares((int)$_GET["key"]);
                if (count($shareResults) == 1): ?>
                <form action="shares.php?save=save" method="post" id="editForm">
                    <input type="hidden" name="shareKey" value="<?php echo $shareResults[0]["shareKey"]; ?>">
                    <div class="panel-heading">
                        <h2 class="panel-title"><?php echo (isset($_GET["new"]) ? "Add" : "Edit"); ?> Share</h2>
                    </div>
                    <div class="panel-body">
                        <div class="form-group clearfix">
                            <label class="col-sm-2 control-label" for="shareName">
                                Share Name
                            </label>
                            <div class="col-sm-10">
                                <input class="form-control" name="shareName" id="shareName" type="text" value="<?php echo $shareResults[0]["shareName"]; ?>" pattern="[.a-zA-Z0-9_]+" required="required">

                                <span class="help-block">Enter a name for this share. It will be used to connect to it.</span>
                            </div>
                        </div>
                        <div class="form-group clearfix">
                            <label class="col-sm-2 control-label" for="localPath">
                                Server Path
                            </label>
                            <div class="col-sm-10">
                                <input class="form-control" name="localPath" id="localPath" type="text" value="<?php echo $shareResults[0]["localPath"]; ?>" pattern=".+" required="required">

                                <span class="help-block">Select the local path that the share will serve.</span>
                            </div>
                        </div>
                        <div class="form-group clearfix">
                            <label class="col-sm-2 control-label">
                                Permitted Users
                            </label>
                            <div class="col-sm-10">
                                <div id="shareUsers"></div>

                                <span class="help-block">Choose the users that will have access to this share.</span>
                            </div>
                        </div>
                        <?php if (!isset($_GET["new"])): ?>
                        <div class="form-group clearfix">
                            <label class="col-sm-2 control-label">
                                Last Changed
                            </label>
                            <div class="col-sm-10">
                                <?php echo $shareResults[0]["updtUser"]; ?> at <script type="text/javascript">
                                    var updtTmStmp = new Date(<?php echo $shareResults[0]["updtTmStmp"]; ?> * 1000);
                                    document.write(updtTmStmp.toLocaleString());
                                </script>
                            </div>
                        </div>
                        <?php endif; ?>
                    </div>
                    <button type="submit" id="btnSave" class="btn btn-primary saveBtn" name="Save"><i class="far fa-save"></i> <?php echo (isset($_GET["new"]) ? "Add" : "Save"); ?> </button>
                </form>
                <script type="text/javascript">
                    var btnSave = document.getElementById('btnSave');
                    var inputs = document.getElementsByTagName('input');
                    for (var i = 0; i < inputs.length; i++) {
                        inputs[i].onkeydown = inputs[i].onkeyup = inputs[i].oninput = function() {
                            var badInputs = 0;
                            for (var j = 0; j < inputs.length; j++) {
                                if (inputs[j].value == "")
                                    badInputs++;
                            }
                            if (badInputs == 0)
                                btnSave.disabled = false;
                            else
                                btnSave.disabled = true;
                        }
                    }

                    var shareUsers = [];
                    shareUsers = <?php echo json_encode($shareResults[0]["users"], JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP); ?>;
                    for (var i = 0; i < shareUsers.length; i++) {
                        shareUsers[i].isOrig = true;
                    }
                    var allUsers = [];
                    allUsers = <?php echo json_encode(get_users(), JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP); ?>;
                    var shareUsersDiv = document.getElementById('shareUsers');
                    function refreshUsers() {
                        shareUsersDiv.innerHTML = "";
                        shareUsers.forEach(function (value, idx) {
                            if (value.deleted)
                                return;

                            var currUser = document.createElement('div');
                            currUser.className = 'shareUser clearfix';
                            var userName = document.createElement('div');
                            userName.className = 'col-sm-6';
                            userName.innerHTML = value.userName + ' (' + value.email + ')';
                            currUser.appendChild(userName);
                            var permsdiv = document.createElement('div');
                            permsdiv.className = 'col-sm-5';
                            var readPerm = document.createElement('div');
                            readPerm.className = 'col-sm-3';
                            readPerm.innerHTML = 'Read ';
                            var readBox = document.createElement('input');
                            readBox.type = 'checkbox';
                            readBox.checked = value.defaultPerms.indexOf('r') > -1;
                            readBox.onclick = function (ev) {
                                if (readBox.checked)
                                    value.defaultPerms += 'r';
                                else
                                    value.defaultPerms = value.defaultPerms.replace('r', '');
                            };
                            readPerm.appendChild(readBox);
                            permsdiv.appendChild(readPerm);
                            var writePerm = document.createElement('div');
                            writePerm.className = 'col-sm-3';
                            writePerm.innerHTML = 'Write ';
                            var writeBox = document.createElement('input');
                            writeBox.type = 'checkbox';
                            writeBox.checked = value.defaultPerms.indexOf('w') > -1;
                            writeBox.onclick = function (ev) {
                                if (writeBox.checked)
                                    value.defaultPerms += 'w';
                                else
                                    value.defaultPerms = value.defaultPerms.replace('w', '');
                            };
                            writePerm.appendChild(writeBox);
                            permsdiv.appendChild(writePerm);
                            var executePerm = document.createElement('div');
                            executePerm.className = 'col-sm-3';
                            executePerm.innerHTML = 'Execute ';
                            var executeBox = document.createElement('input');
                            executeBox.type = 'checkbox';
                            executeBox.checked = value.defaultPerms.indexOf('x') > -1;
                            executeBox.onclick = function (ev) {
                                if (executeBox.checked)
                                    value.defaultPerms += 'x';
                                else
                                    value.defaultPerms = value.defaultPerms.replace('x', '');
                            };
                            executePerm.appendChild(executeBox);
                            permsdiv.appendChild(executePerm);
                            var ownerPerm = document.createElement('div');
                            ownerPerm.className = 'col-sm-3';
                            ownerPerm.innerHTML = 'Owner ';
                            var ownerBox = document.createElement('input');
                            ownerBox.type = 'checkbox';
                            ownerBox.checked = value.defaultPerms.indexOf('o') > -1;
                            ownerBox.onclick = function (ev) {
                                if (ownerBox.checked)
                                    value.defaultPerms += 'o';
                                else
                                    value.defaultPerms = value.defaultPerms.replace('o', '');
                            };
                            ownerPerm.appendChild(ownerBox);
                            permsdiv.appendChild(ownerPerm);
                            currUser.appendChild(permsdiv);
                            var deldiv = document.createElement('div');
                            deldiv.className = 'col-sm-1';
                            var delBtn = document.createElement('i');
                            delBtn.className = 'fas fa-trash-alt btnDelUser';
                            delBtn.onclick = function (ev) {
                                if (shareUsers[idx].isOrig)
                                    shareUsers[idx].deleted = true;
                                else
                                    shareUsers.splice(idx, 1);
                                refreshUsers();
                            };
                            deldiv.appendChild(delBtn);
                            currUser.appendChild(deldiv);
                            shareUsersDiv.appendChild(currUser);
                        });

                        var newUserPerms = '';
                        var newUserDiv = document.createElement('div');

                        // Defined early to allow disable from the inline functions.
                        var addBtn = document.createElement('button');

                        newUserDiv.className = 'shareUser clearfix newUser';
                        var userName = document.createElement('div');
                        userName.className = 'col-sm-6';
                        userName.innerHTML = 'Add New User: ';
                        var userSelect = document.createElement('select');
                        var addedUsers = 0;
                        allUsers.forEach(function (value, idx) {
                            for (var i = 0; i < shareUsers.length; i++) {
                                if (shareUsers[i].userKey == value.userKey && !shareUsers[i].deleted)
                                    return;
                            }
                            var userOption = document.createElement('option');
                            userOption.value = idx;
                            userOption.innerHTML = value.userName + ' (' + value.email + ')';
                            userSelect.appendChild(userOption);
                            addedUsers++;
                        });
                        if (addedUsers == 0)
                            return;
                        userName.appendChild(userSelect);
                        newUserDiv.appendChild(userName);
                        var permsdiv = document.createElement('div');
                        permsdiv.className = 'col-sm-5';
                        permsdiv.style.textAlign = 'center';
                        var readPerm = document.createElement('div');
                        readPerm.className = 'col-sm-3';
                        readPerm.innerHTML = 'Read<br>';
                        var readBox = document.createElement('input');
                        readBox.type = 'checkbox';
                        readBox.onclick = function (ev) {
                            if (readBox.checked)
                                newUserPerms += 'r';
                            else
                                newUserPerms = newUserPerms.replace('r', '');

                            addBtn.disabled = (newUserPerms == '');
                        };
                        readPerm.appendChild(readBox);
                        permsdiv.appendChild(readPerm);
                        var writePerm = document.createElement('div');
                        writePerm.className = 'col-sm-3';
                        writePerm.innerHTML = 'Write<br>';
                        var writeBox = document.createElement('input');
                        writeBox.type = 'checkbox';
                        writeBox.onclick = function (ev) {
                            if (writeBox.checked)
                                newUserPerms += 'w';
                            else
                                newUserPerms = newUserPerms.replace('w', '');

                            addBtn.disabled = (newUserPerms == '');
                        };
                        writePerm.appendChild(writeBox);
                        permsdiv.appendChild(writePerm);
                        var executePerm = document.createElement('div');
                        executePerm.className = 'col-sm-3';
                        executePerm.innerHTML = 'Execute<br>';
                        var executeBox = document.createElement('input');
                        executeBox.type = 'checkbox';
                        executeBox.onclick = function (ev) {
                            if (executeBox.checked)
                                newUserPerms += 'x';
                            else
                                newUserPerms = newUserPerms.replace('x', '');

                            addBtn.disabled = (newUserPerms == '');
                        };
                        executePerm.appendChild(executeBox);
                        permsdiv.appendChild(executePerm);
                        var ownerPerm = document.createElement('div');
                        ownerPerm.className = 'col-sm-3';
                        ownerPerm.innerHTML = 'Owner<br>';
                        var ownerBox = document.createElement('input');
                        ownerBox.type = 'checkbox';
                        ownerBox.onclick = function (ev) {
                            if (ownerBox.checked)
                                newUserPerms += 'o';
                            else
                                newUserPerms = newUserPerms.replace('o', '');

                            addBtn.disabled = (newUserPerms == '');
                        };
                        ownerPerm.appendChild(ownerBox);
                        permsdiv.appendChild(ownerPerm);
                        newUserDiv.appendChild(permsdiv);
                        var adddiv = document.createElement('div');
                        adddiv.className = 'col-sm-1';
                        addBtn.type = 'button';
                        addBtn.className = 'btn btn-success';
                        addBtn.disabled = true;
                        addBtn.onclick = function (ev) {
                            var selectedUser = allUsers[userSelect.options[userSelect.selectedIndex].value];
                            selectedUser.defaultPerms = newUserPerms;
                            var foundExisting = false;
                            for (var i = 0; i < shareUsers.length; i++) {
                                if (shareUsers[i].userKey == selectedUser.userKey) {
                                    shareUsers[i] = selectedUser;
                                    shareUsers[i].isOrig = true;
                                    shareUsers[i].deleted = false;
                                    foundExisting = true;
                                    break;
                                }
                            }
                            if (!foundExisting)
                                shareUsers.push(selectedUser);
                            refreshUsers();
                        };
                        var addIcon = document.createElement('i');
                        addIcon.className = 'fas fa-plus';
                        addBtn.appendChild(addIcon);
                        adddiv.appendChild(addBtn);
                        newUserDiv.appendChild(adddiv);
                        shareUsersDiv.appendChild(newUserDiv);
                    }
                    refreshUsers();

                    var theForm = document.getElementById('editForm');
                    theForm.onsubmit = function (ev) {
                        var hiddenInput = document.createElement('input');
                        hiddenInput.type = 'hidden';
                        hiddenInput.name = 'shareUsers';
                        hiddenInput.value = JSON.stringify(shareUsers);
                        theForm.appendChild(hiddenInput);

                        return true;
                    }
                </script>
                <?php else: ?>
                <h3>Invalid share requested.</h3>
                <?php http_response_code(404); ?>
                <?php endif; ?>
            <?php else: ?>
            <div class="panel-heading">
                <h2 class="panel-title">Existing Shares</h2>
                <a href="?new=new" class="btn btn-success addBtn"><i class="fas fa-plus"></i> Add</a>
            </div>
            <div class="panel-body">
                <table class="table table-striped">
                    <thead>
                    <tr>
                        <th>Share Name</th>
                        <th>Server Path</th>
                        <th style="width: 1px;">Edit</th>
                    </tr>
                    </thead>
                    <tbody>
                    <?php foreach(get_shares() as $share): ?>
                        <tr>
                            <td><?php echo $share["shareName"]; ?></td>
                            <td><?php echo $share["localPath"]; ?></td>
                            <td><a href="shares.php?key=<?php echo $share["shareKey"]; ?>"><i class="fas fa-edit editBtn"></i></a></td>
                        </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
                <?php if (!isset($share)): ?>
                    <div class="no-data">There are no existing shares.</div>
                <?php endif; ?>
            </div>
            <?php endif; ?>
        </div>
        <?php else: ?>
        <h3>You are not allowed to access this page.</h3>
        <?php http_response_code(403);
              endif; ?>
    </div> <!-- /container -->
</body>
</html>