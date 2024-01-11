the flatc program will be copied here when built via cads-edge.
In cads root directory:
sh> mkdir bin

Install dotnet 7 from MS if running on Jetson Nano

IN ~/bash_aliases
export DOTNET_ROOT=$HOME/.dotnet
PATH=$PATH:$HOME/.dotnet
PATH=$PATH:$HOME/.dotnet/tools


Install nvm if on the nano otherwise whatever
https://github.com/nvm-sh/nvm

using nvm install node 16.20.0 The version may not important but currently works.
nvm install 16.20.0

From flatbuffers-typescript directory
npm run build




From caas-gui directory
dotnet tool install --global dotnet-ef --version=7.0.5



As user postgres:
sh> createuser --interactive
sh> createdb caas
sh> psql -d caas
psql> create schema caas authorization caas;
psql> alter user caas with password 'caas';

As user running caas-gui
sh> dotnet ef migrations add Init
sh> dotnet ef database update

Generate development localhost cert for https on ArchLinux use mkcert
sh> mkcert -install

In caas directory:
mkcert localhost


