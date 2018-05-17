<?php
/*
 * SSNFS WebPanel v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 *
 */ ?>
<!-- Static navbar -->
<nav class="navbar navbar-default">
    <div class="container-fluid">
        <div class="navbar-header">
            <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#navbar" aria-expanded="false" aria-controls="navbar">
                <span class="sr-only">Toggle navigation</span>
                <span class="icon-bar"></span>
                <span class="icon-bar"></span>
                <span class="icon-bar"></span>
            </button>
            <b><a class="navbar-brand" href="#">SSNFS</a></b>
        </div>
        <div id="navbar" class="navbar-collapse collapse">
            <ul class="nav navbar-nav">
                <li <?php echo ($_SERVER['PATH_INFO'] === '/' || strpos($_SERVER['PATH_INFO'], '/index.php') === 0) ? 'class="active"' : ''; ?> ><a href="/">Home</a></li>
                <li class="dropdown <?php echo (strpos($_SERVER['PATH_INFO'], '/connected.php') === 0) ? 'active' : ''; ?>">
                    <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false">Status <span class="caret"></span></a>
                    <ul class="dropdown-menu">
                        <li><a href="/connected.php">Connected</a></li>
                    </ul>
                </li>
                <li class="dropdown <?php echo (strpos($_SERVER['PATH_INFO'], '/settings.php') === 0 ||
                    strpos($_SERVER['PATH_INFO'], '/users.php') === 0 ||
                    strpos($_SERVER['PATH_INFO'], '/shares.php') === 0 ||
                    strpos($_SERVER['PATH_INFO'], '/computers.php') === 0) ? 'active' : ''; ?>">
                    <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false">Manage <span class="caret"></span></a>
                    <ul class="dropdown-menu">
                        <li><a href="/settings.php">Global Settings</a></li>
                        <li><a href="/users.php">Users</a></li>
                        <li><a href="/shares.php">Shares</a></li>
                        <li><a href="/computers.php">Computers</a></li>
                    </ul>
                </li>
            </ul>
            <ul class="nav navbar-nav navbar-right">
                <li><a href="/logout.php">Logout</a></li>
            </ul>
        </div><!--/.nav-collapse -->
    </div><!--/.container-fluid -->
</nav>

<script type="text/javascript">
    var resize_timeout;
    $(window).on('resize orientationchange', function(){
        clearTimeout(resize_timeout);

        resize_timeout = setTimeout(function(){
            var open_menu = $('.dropdown.open a');
            if (open_menu.length) {
                open_menu.click();
            }
        }, 50);
    });

    // Add animations to Bootstrap dropdown menus when expanding.
    $('.dropdown').on('show.bs.dropdown', function() {
        if ($('.navbar-toggle').css('display') === "none")
            $(this).find('.dropdown-menu').first().stop(true, true).slideDown(200);
    });
    $('.dropdown').on('hide.bs.dropdown', function() {
        if ($('.navbar-toggle').css('display') === "none")
            $(this).find('.dropdown-menu').first().stop(true, true).slideUp(200, function () {
                $('.dropdown ul').css('display', '')
            });
    });
</script>