Installation notes.

Init new project with:
dotnet new blazorserver -o cads-gui

cads-gui is the directory name

Add sqlite:
dotnet add package Microsoft.EntityFrameworkCore.sqlite

Add Scripting: Not really required but useful for easying prototyping
dotnet add package Microsoft.CodeAnalysis.CSharp.Scripting


To run on different address:port use --urls option
dotnet run --urls=http://noxmlhere.com:60175

Installing dotnet on Ubuntu
wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb


Deploying to Azure.
From Azure Portal - https://portal.azure.com find link to "App Services"
Create new app from App Services
Fill in the form. Notable options :-
Resourse Group : Used an existing wondersystems group from dropdown list.
Runtime Stack : .Net Core 3.1
Operating System: Linux (Because development is in Linux and WServer is Linux)
Use Free version

Migrations(Linux)
Install Tools
dotnet tool install --global dotnet-ef
dotnet add package Microsoft.EntityFrameworkCore.Tools 
export PATH="$PATH:$HOME/.dotnet/tools/"
Insure sqlite packages in csproj are the version as Microsoft.EntityFrameworkCore.Tools 
Needs connection string to determine the database name. Migrations cannot handle computing the connection string, so read directly from appsettings.json



Initial Database
dotnet ef migrations add InitialCreate
dotnet ef database update
dotnet ef database update -- --environment fmg






