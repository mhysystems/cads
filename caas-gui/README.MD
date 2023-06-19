As privilged user:
sh> createuser --interactive
sh> createdb caas
psql> create schema caas authorization caas;

As user running caas-gui
sh> dotnet ef migrations add Init
sh> dotnet ef database update