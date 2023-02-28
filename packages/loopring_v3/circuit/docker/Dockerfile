# FROM registry.bihutest.com/base/zkp-base:1.0.0-1
FROM registry.degate.space/base/zkp-base:1.0.0-1

ARG VERSION

ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8

ADD ./build/ /degate/zkpworker/build/
ADD docker/entrypoint.sh /degate/zkpworker/
ADD docker/entrypoint-host.sh /degate/zkpworker/
ADD module_config.tpl /data/module_config.tpl
RUN chmod +x /degate/zkpworker/*.sh
RUN mkdir -p /degate/zkpworker/s3_data

WORKDIR /degate/zkpworker/

ENTRYPOINT ["./entrypoint.sh"]
