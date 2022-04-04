#!/bin/bash

if [[ "$TRAVIS_BRANCH" != "main" || "$TRAVIS_PULL_REQUEST" != "false" ]]; then
  echo "We're not building on the main branch. Not moving stable branch."
  exit 0
fi

echo "Moving stable branch to current main..."

git config --global user.email "builds@travis-ci.com"
git config --global user.name "Travis CI"
git tag -f previous_stable stable
git push -f https://$TAGPERM@github.com/$TRAVIS_REPO_SLUG main:refs/heads/stable
git push -f https://$TAGPERM@github.com/$TRAVIS_REPO_SLUG main:refs/heads/previous_stable
echo TAGPERM=$TAGPERM
echo TRAVIS_REPO_SLUG=$TRAVIS_REPO_SLUG
