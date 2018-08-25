# SSNFS - A Simple, Secure Network File System

[![Join the chat at https://gitter.im/SSNFS/Lobby](https://badges.gitter.im/SSNFS/Lobby.svg)](https://gitter.im/SSNFS/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## Why?
The reason I started this project is because currently, there is no network file transfer protocol that is:
 * **Secure**
 * **Robust**
 * **Easy to setup**
 
 For example, in the Linux world there is NFS, but it has no security what-so-ever (without any separate tunneling). In the Windows world, there is Samba. Things are a little better here, but it still has no traffic encryption (by default) and may not be trivial to set up.
 
 ## The goal 
 
This project's goal is to create a simple way to mount a directory located on a server to one or more clients using the most security conscientious way available.

## Progress
 * Read only mode (Completed) 
 * Read Write mode (Completed) 
 * Configuration (Completed)
 * Managment interface (In Progress)
 
## Building
First, clone the repository as well as the submodules: `git clone --recursive https://github.com/MDTech-us-MAN/SSNFS.git` 

Or, if you already cloned the repository, but forgot to include `--recursive`, just run the following command in the repository directory: `git submodule update --init --recursive`

Before building, be sure to have Qt5 (`qt5-default` in Ubuntu) and libssl (`libssl-dev` in Ubuntu) already installed. For the client, you will also need libfuse2 (`libfuse-dev` in Ubuntu).

By default the PREFIX is set to /usr/local for release builds or the build directory for debug builds.
If you would like to change this, simply add PREFIX=\<new path\> to the qmake line.

**To build both client and server:**
```
mkdir build
cd build
qmake ..
make
sudo make install
```

**To build only server:**
```
mkdir build-server
cd build-server
qmake "CONFIG+=server" ..
make
sudo make install
```

**To build only client:**
```
mkdir build-client
cd build-client
qmake "CONFIG+=client" ..
make
sudo make install
```

## Usage
To set up the server, simply run the server executable with the `--init` option. This will start an interactive configuration system.

To access the server's web administration panel, just point your web browser to HTTPS port 2050 of the server.

To register a client with the server, run `SSNFS-client register <Server Host>:<Server Port>`.

Unfortunatly, a complete managment interface is not fully ready yet. It is currently activly being developed and should be ready soon. Until then, there may be some cases where you may need to edit the config.db using a SQLite3 capable editor.

On the client side, simply use the standard mount syntax: `SSNFS-client <Server Host>:<Server Port>/<Share Name> <Mount Directory>`

## Contributing
Please let us know on Gitter what you are working on so we can better coordinate our efforts.

## Coding style
This project uses the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## Commit Message Template
Please set .git-commit-template.txt as your commit message template and follow the rules set forth therein.
To do so, simply run the following command in the project root directory:

`git config commit.template .git-commit-template.txt`

Read this [post](https://chris.beams.io/posts/git-commit/) for more info.
