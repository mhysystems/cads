As privilged user:
sh> createuser --interactive
sh> createdb caas
psql> create schema caas authorization caas;
psql> alter user caas with password 'caas';

As user running caas-gui
sh> dotnet ef migrations add Init
sh> dotnet ef database update