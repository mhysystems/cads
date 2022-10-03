export default {
  root: "cads-flatbuffers",
  build:  {
    lib: {
      entry: 'main.ts',
      name: 'cadsFlatbuffersPlot',
      formats:['es'],
      fileName:(format) => `cads-flatbuffers-plot.${format}.js`
    },
    outDir : "../../wwwroot"
  }
}