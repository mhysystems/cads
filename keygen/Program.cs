using System.CommandLine;
using System.Security.Cryptography;
using System.Text;

namespace Caas;

public class KeyGen
{

  public static string Encrypt(string content, string password)
  {
    var bytes = Encoding.UTF8.GetBytes(content);

    using var crypt = Aes.Create();

    crypt.Key = SHA256.HashData(Encoding.UTF8.GetBytes(password));
    crypt.GenerateIV();

    using var memoryStream = new MemoryStream();
    using var cryptoStream = new CryptoStream(memoryStream, crypt.CreateEncryptor(), CryptoStreamMode.Write);

    cryptoStream.Write(bytes, 0, bytes.Length);
    cryptoStream.FlushFinalBlock();

    string base64IV = Convert.ToBase64String(crypt.IV);
    string base64Ciphertext = Convert.ToBase64String(memoryStream.ToArray());

    var based = Encoding.UTF8.GetBytes(base64IV + "!" + base64Ciphertext);
    return System.Convert.ToBase64String(based);

  }

  public static string Decrypt(string IVcontent, string password)
  {
    try
    {
      var IVbytes = Convert.FromBase64String(IVcontent);
      var content = Encoding.UTF8.GetString(IVbytes);

      var name = content.Split("!");

      if (name.Length != 2) return "";

      var IV = Convert.FromBase64String(name[0]);
      var bytes = Convert.FromBase64String(name[1]);

      using var crypt = Aes.Create();
      using var memoryStream = new MemoryStream();

      crypt.Key = SHA256.HashData(Encoding.UTF8.GetBytes(password));
      crypt.IV = IV;

      using var cryptoStream = new CryptoStream(memoryStream, crypt.CreateDecryptor(), CryptoStreamMode.Write);
      cryptoStream.Write(bytes, 0, bytes.Length);
      cryptoStream.FlushFinalBlock();

      return Encoding.UTF8.GetString(memoryStream.ToArray());
    }
    catch (Exception)
    {
      return "";
    }

  }

  static void Main(string[] args)
  {
    var rootCommand = new RootCommand("Encode serial number");
    var keyOption = new Option<string>("--key", "Base64 Shared Key");
    var serialOption = new Option<string>("--serial", "Serial to encode or decode");

    var encodeCommand = new Command("encode", "Encode serial with key")
{
    keyOption,
    serialOption
};

    encodeCommand.SetHandler((key, serial) =>
    {
      Console.WriteLine(Encrypt(serial, key));
    },
    keyOption, serialOption);

    var decodeCommand = new Command("decode", "Encode serial with key")
{
    keyOption,
    serialOption
};

    decodeCommand.SetHandler((key, serial) =>
    {
      Console.WriteLine(Decrypt(serial, key));
    },
    keyOption, serialOption);


    rootCommand.AddCommand(encodeCommand);
    rootCommand.AddCommand(decodeCommand);

    rootCommand.Invoke(args);
  }

}


