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

Be sure that the post-merge hook is executed before compiling. See the [Contributing](#Contributing) section for more on this.

By default the PREFIX is set to /usr/local for release builds or the build directory for debug builds.
If you would like to change this, simply add PREFIX=\<new path\> to the qmake line.

Server:
```
mkdir build-server
cd build-server
qmake "CONFIG+=server" ..
make
sudo make install
```

Client:
```
mkdir build-client
cd build-client
qmake "CONFIG+=client" ..
make
sudo make install
```

## Usage

Unfortunatly, a proper managment interface is not ready yet. It is currently activly being developed and should be ready soon. Until then, please edit the config.db using a SQLite3 capable editor. To properly use the FS, you must at least edit the Settings table and the Shares table.

On the client side, simply use the standard mount syntax: SSNFS-client <Server Host>:<Server Port>/<Share Name> <Mount Directory>

## Contributing

Before starting, please install the sqlite3 command line tool. (`apt install sqlite3`)

After cloning the repository, execute `sh addHooks.sh` to add the required hooks.

Finally, manually execute the post-merge hook the first time: `sh post-merge`

Please let us know on Gitter what you are working on so we can better coordinate our efforts.

## Coding style
This project uses the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## Commit Message Template
Please set .git-commit-template.txt as your commit message template and follow the rules set forth therein.
To do so, simply run the following command in the project root directory:

`git config commit.template .git-commit-template.txt`

Read this [post](https://chris.beams.io/posts/git-commit/) for more info.
