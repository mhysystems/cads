using System.Text.Json;
using System.Text.Json.Serialization;

public class TimeZoneInfoConverter : JsonConverter<TimeZoneInfo>
{
    public override TimeZoneInfo Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var timezone = reader.GetString() ?? "/Etc/UTC";
        return TimeZoneInfo.FindSystemTimeZoneById(timezone);
    }

    public override void Write(Utf8JsonWriter writer, TimeZoneInfo value, JsonSerializerOptions options)
    {
        writer.WriteStringValue(value.ToString());
    }
}