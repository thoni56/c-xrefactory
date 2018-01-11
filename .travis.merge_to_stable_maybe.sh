#!/bin/bash

set -x

if [[ "$TRAVIS_BRANCH" != "master" || "$TRAVIS_PULL_REQUEST" != "false" ]]; then
  echo "We're not building on the master branch. Not merging to stable."
  exit 0
fi

echo "Merging to stable..."

git config --global user.email "builds@travis-ci.com"
git config --global user.name "Travis CI"
git branch -v -f stable master
git checkout stable
git status
echo git push https://$TAGPERM@github.com/$TRAVIS_REPO_SLUG
git push https://$TAGPERM@github.com/$TRAVIS_REPO_SLUG
