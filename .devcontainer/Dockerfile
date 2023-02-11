FROM espressif/idf:v4.4
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc

RUN apt-get update && apt-get install -y -qq locales
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
 && sed -i -e 's/# pt_BR.UTF-8 UTF-8/pt_BR.UTF-8 UTF-8/' /etc/locale.gen \
 && locale-gen
