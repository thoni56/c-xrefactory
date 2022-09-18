# Architecture as code 2.0

This directory contains [Structurizr](https://structurizr.com/) DSL files for c-xrefactory.

They (will) model all important aspects of the C-xrefactory system in a C4 model fasion.
Structurizr makes it possible to navigate them and generate images of the views to include in documentation.

The easiest way to get at that model is to use the Structurizr Docker image.
One one command necessary (if you have Docker installed):

    docker run -it --rm -v $PWD:/usr/local/structurizr -p 8080:8080 structurizr/lite

This assumes that you are in this directory, otherwise you can point Structurizr to it by replacing `$PWD` with the path to this directory.

You can also map the ports differently, obviously.
But if you have ran the command above, just navigate to `localhost:8080` and the model will show up.
