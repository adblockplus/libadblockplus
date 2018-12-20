FROM ubuntu:18.04

MAINTAINER Krystian Zlomek <k.zlomek@adblockplus.org>

RUN apt-get update -qyy && \
    apt-get install -qyy \
    sudo \
    dumb-init \
    curl

RUN curl -L https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh | bash

COPY pin-gitlab-runner.pref /etc/apt/preferences.d/pin-gitlab-runner.pref

RUN apt-get install -qyy gitlab-runner

RUN adduser --gecos "" --disabled-password ci_user && \
    usermod -aG sudo ci_user && \
    echo "ci_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers && \
    mkdir /opt/ci && \
    chown -R ci_user:ci_user /opt/ci

ENTRYPOINT ["/usr/bin/dumb-init", "--"]

CMD ["gitlab-runner", "run", "--working-directory", "/opt/ci", "--user", "ci_user"]
