# SSNFS - A Simple, Secure Network File System

[![Join the chat at https://gitter.im/SSNFS/Lobby](https://badges.gitter.im/SSNFS/Lobby.svg)](https://gitter.im/SSNFS/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## Why?
The reason I started this project is because currently, there is no network file transfer protocol that is:
 * **Secure**
 * **Robust**
 * **Easy to setup**
 
 For example, in the Linux world there is NFS, but it has no security what-so-ever (without any separate tunneling). In the Windows world, there is Samba. Things are a little better here, but it still has no traffic encryption (by default) and may not be trivial to set up.
 
 ## The goal 
 
This project's goal is to create a simple way to mount a directory located on a server to one or more clients used the most security conscientious way available.

## Progress

 * Read only mode (Completed) 
 * Read Write mode (Completed) 
 * Configuration (Completed)
 * Managment interface (In Progress)

## Coding style
This project uses the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), with the exception of using tabs instead of spaces.

## Commit Message Template
Please set .git-commit-template.txt as your commit message template and follow the rules.
To do so, simply run below command in project root directory:

`git config commit.template .git-commit-template.txt`

Read this [post](https://chris.beams.io/posts/git-commit/) for more info.
