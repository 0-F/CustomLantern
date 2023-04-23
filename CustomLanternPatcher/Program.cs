using CustomLanternPatcher;
using FluentArgs;
using System.Globalization;

CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;

FluentArgsBuilder.New()
    .WithApplicationDescription(
        "This application modifies sfxbnd_commoneffects.ffxbnd.dcx to change the light color of the lantern.")
    .RegisterHelpFlag("-h", "/h", "--help", "/help", "-?", "/?")
    .Parameter("-i", "--input")
        .WithDescription("Input file.")
        .IsRequired()
    .Parameter("-o", "--output")
        .WithDescription("Output modded file.")
        .IsRequired()
    .Parameter("-c", "--config")
        .WithDescription("Config file.")
        .IsOptionalWithDefault("config.ini")
    .Parameter("-cl", "--config-lantern")
        .WithDescription("Config file that contains new values of the lantern light.")
        .IsOptionalWithDefault("custom-lantern.ini")
    .Call(configLantern => config => outputFile => inputFile =>
    {
        Settings cfg = new(config);
        LanternSettings lanternCfg = new(configLantern);
        Patcher.Patch(inputFile, outputFile, cfg, lanternCfg);
    })
    .Parse(args);
