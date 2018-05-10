#!/bin/sh

chmod -v +x pre-commit
ln -v -s ../../pre-commit .git/hooks/pre-commit

chmod -v +x post-commit
ln -v -s ../../post-commit .git/hooks/post-commit

chmod -v +x post-merge
ln -v -s ../../post-merge .git/hooks/post-merge
ln -v -s ../../post-checkout .git/hooks/post-checkout
