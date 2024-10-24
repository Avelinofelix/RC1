# -*- mode: ruby -*-
# vi: set ft=ruby :-

# RCI, 2024. Docente: joaojdacosta@gmail.com

Vagrant.configure("2") do |config|
  # config.vm.box = "ktr/mininet"
  config.ssh.insert_key = false
  config.vbguest.auto_update = false

  # Define the webserver
  config.vm.define "webserver" do |web_config|
    web_config.vm.box = "ubuntu/trusty64"
    web_config.vm.hostname = "webserver"
    web_config.vm.network "private_network", ip: "192.168.56.21"
    web_config.vm.network "forwarded_port", guest: 80, host: 8080
    web_config.vm.provider "virtualbox" do |vb|
      vb.name = "webserver"
      opts = ["modifyvm", :id, "--natdnshostresolver1", "on"]
      vb.customize opts
      vb.memory = "256"
    end # do vb
    web_config.vm.provision "shell", path:"bootstrap_web.sh"
  end # do web_config

  # Define the client
  config.vm.define "client" do |client_config|
    client_config.vm.box = "ubuntu/trusty64"
    client_config.vm.hostname = "client"
    client_config.vm.network "private_network", ip: "192.168.56.11"
    client_config.vm.provider "virtualbox" do |vb|
      vb.name = "client"
      opts = ["modifyvm", :id, "--natdnshostresolver1", "on"]
      vb.customize opts
      vb.memory = "256"
    end # do vb
  end # do client_config
end # do config
