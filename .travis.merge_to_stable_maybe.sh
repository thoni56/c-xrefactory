#!/bin/bash

if [[ "$TRAVIS_BRANCH" != "master" || "$TRAVIS_PULL_REQUEST" != "false" ]]; then
  echo "We're not building on the master branch. Not moving stable branch."
  exit 0
fi

echo "Moving stable branch to current build..."

git config --global user.email "builds@travis-ci.com"
git config --global user.name "Travis CI"
git push -f https://$TAGPERM@github.com/$TRAVIS_REPO_SLUG master:refs/heads/stable
