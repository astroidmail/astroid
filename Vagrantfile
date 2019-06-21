# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.provider "virtualbox" do |v|
    config.vm.box = "archlinux/archlinux"
    v.memory = (3 * 1024).to_s
    v.cpus   = 4
  end

  config.ssh.forward_x11 = true

  config.vm.provision "shell", inline: <<-SHELL
    pacman -Syu --noconfirm

    pacman -S --noconfirm --needed cmake ninja ccache git gcc notmuch-runtime glibmm gtkmm3 vte3 boost libsass libpeas ruby-ronn pkgconf webkit2gtk protobuf gobject-introspection xorg-xauth xorg-xclock cmark python-gobject ipython gvim

    cat > /etc/profile.d/astroid.sh <<EOL
      export ASTROID_DIR=/vagrant
      export GI_TYPELIB_PATH=/home/vagrant/build
EOL

    sed -e "s|#X11Forwarding no|X11Forwarding yes|" -i /etc/ssh/sshd_config

  SHELL

  config.vm.provision "shell", privileged: false, inline: <<-SHELL

    mkdir -p /home/vagrant/build
    cd /home/vagrant/build
    cmake -GNinja -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_FLAGS=-fdiagnostics-color=always /vagrant

  SHELL
end

