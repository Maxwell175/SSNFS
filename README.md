# SSNFS - A Simple, Secure Network File System

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
 * Read Write mode (In Progress) 
 * Configuration 
 * Managment interface?

## Coding style
This project uses the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), with the exception of using tabs instead of spaces.
