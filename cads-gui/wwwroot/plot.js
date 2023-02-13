import { plot_data, ByteBuffer } from './cads-flatbuffers-plot.es.js'

function generate_ploty_z_samples(typedArray, columns) {

  const z_samples = [];

  for (let i = 0; i < typedArray.length; i += columns) {
    z_samples.push(typedArray.subarray(i, i + columns));
  }

  return z_samples;
}

function mk_2dArray(rows, columns, zMin) {

  const z_samples = [];

  for (let i = 0; i < rows; i++) {
    z_samples.push(new Array(columns).fill(zMin));
  }

  return z_samples;
}

function mk_BottomSurface(top, bottom) {


  for (let j = 1; j < top.length-1; j++) {
    for (let i = 1; i < top[j].length-1; i++) {
      bottom[j][i] = Math.abs(bottom[j][i] - top[j][i] + 1.0);
    
    }
  }

}

function generate_ploty_y_axis(y_sample_start, num_plot_y_samples, num_y_samples, y_sample_len) {

  const mod = (x, y) => x; //(x,y) => ((x % y) + y) % y;

  const dbg = [...Array(num_plot_y_samples).keys()].map(x => mod(y_sample_start + x, num_y_samples) * y_sample_len);
  console.log(`yaxis range: ${dbg[0]},${dbg[dbg.length - 1]} length: ${dbg.length}`)

  return dbg;

}



// Cannot get the profile buffer out of JS into Blazor, so use handles
const handles = [];


export function init(
  z_min,
  z_max,
  x_res,
  y_res,
  colour_scale,
  belt_name,
  htmlelement) {

  let color_scale = typeof (colour_scale) == 'string' ? colour_scale : colour_scale.map(({ item1, item2 }) => [item1, item2]);
  const cut_y = 9.0;
  let sample_y = 0;

  //var w = new Worker('background_download.js');

  //const buffer = new SharedArrayBuffer(buffer_length);

  //new Float32Array(buffer).fill(-0);
  // w.postMessage({ b: buffer, f: filename, s: num_plot_y_samples});

  //const bottom_plane = [];


  //for (let i = 0; i < num_plot_y_samples; i++) {
  //bottom_plane.push(new Float32Array(num_x_samples).fill(z_min));
  //}

  //const o = document.querySelector('#'+htmlelement).getBoundingClientRect();

  //const hh = (o.right - o.left)*(belt_width / plot_y_length);
  //console.log((o.right - o.left),o.top - o.bottom, belt_width, plot_y_length);
  const defaultEyePosition = { x: 0.5, y: 0, z: 1.2 };
  const layout = {
    // title: '➤➤➤➤➤➤➤➤➤➤',
    autosize: true,
    //  height:hh,
    margin: {
      l: 0,
      r: 0,
      b: 0,
      t: 0,
    },
    //unirevision : "true",
    scene: {
      camera: {
        center: {
          x: 0,
          y: 0,
          z: 0
        },
        eye: { ...defaultEyePosition }
      },

      zaxis: {
        //showspikes: false,
        //spikemode :"across",

        range: [z_min, z_max]
      },

      yaxis: {
        showspikes: true,
        spikecolor: "magenta",
        spikemode: "across",
        spikethickness: 5,
        // range : [0,num_plot_y_samples*y_sample_len]
      },

      xaxis: {
        showspikes: true,
        spikecolor: "magenta",
        spikethickness: 5,
        spikemode: "across",
        //  range:[x_start,x_end]
      },

      aspectratio: {
        //y: plot_y_length / belt_width,
        x: 1,
        //z: z_max / (belt_width*1000)
      },
    }, // end scene
  };

  const layout2 = {
    //height:200,
    autosize: true,
    yaxis: {
      range: [z_min, z_max],
      side: 'right'
    },
    xaxis: {
      // range: [x_start, x_end],

    },

    shapes: [{
      type: 'line',
      //x0: x_start,
      y0: 29.5,
      //x1 : x_end,
      y1: 29.5,
      line: {
        color: 'rgb(50, 171, 96)',
        width: 2
      }

    }],

    margin: {
      l: 20,
      r: 20,
      b: 20,
      t: 20,
    },

    aspectratio: {
      x: 1,
      //y: z_max / (belt_width*1000),
    },
  };

  const config = {
    displaylogo: false,
    displayModeBar: true
  };


  function resize_plot() {
    //  const o = document.querySelector('#'+htmlelement).getBoundingClientRect();

    //  const hh = (o.right - o.left)*(belt_width / plot_y_length);
    //  layout.height = hh;
    //  layout.width = o.right-o.left;
    //  layout.scene.camera.eye.z = 2.0;
    //console.log("dim",layout.width ,layout.height);

    //(async _ => {await null; Plotly.update(htmlelement, data, layout);})();
    //(async _ => {await null; Plotly.relayout(htmlelement, layout);})();
    Plotly.purge(htmlelement);
    //(async _ => {await null; Plotly.update(htmlelement, trace, ulayout,0);})();
    Plotly.react(htmlelement, data, layout, config);

    const { width, height } = document.querySelector(`#${htmlelement}-profile`).getBoundingClientRect();
    const scale = width / height > 3 ? 1 : 4.5;

    layout2.yaxis.range = [z_min, z_max * scale];

    Plotly.purge(htmlelement + "-profile");
    Plotly.react(htmlelement + "-profile", data2, layout2, config);
    var myPlot = document.getElementById(htmlelement);


    myPlot.on('plotly_click', function (e) {
      console.debug(`plotly_click`);
      const y = e.points[0].pointNumber[1];
      data2[0].y = data[0].z[y];
      data2[1].y = data2[0].y.map(y => 16.2 * (y > cut_y));
      Plotly.react(htmlelement + "-profile", data2, layout2, config);

    });
  }


  function plot_change_colorscale(c) {
    c = c.map(({ item1, item2 }) => [item1, item2]);

    console.debug(`plot_change_colorscale(${c})`);
    data[0].colorscale = c;
    data[1].colorscale = c;
    Plotly.react(htmlelement, data, layout);

  }

  function plot_overlay(x, width, y, length) {
    console.debug(`plot_overlay(${x},${width},${y},${length})`);
    let z = [];
    const x_off = data[0].x[0] * 1000;
    const x_off_end = data[0].x.slice(-1) * 1000;
    let y_off_end = data[0].y.slice(-1) * 1000;
    let y_off_start = data[0].y[0] * 1000;
    const y_off = Math.min(y_off_start, y_off_end);

    let indexX = Math.floor((x - x_off) / x_res);
    let lengthX = Math.floor(width / x_res);

    if (indexX < 0) indexX = 0;
    if (indexX + lengthX > data[0].x.length) lengthX = data[0].x.length - indexX;

    let indexY = Math.floor((y - y_off) / y_res);
    let lengthY = Math.floor(length / y_res);

    if (indexY < 0) indexY = 0;
    if (indexY + lengthY > data[0].z.length) lengthY = data[0].z.length - indexY;

    for (let j = indexY; j < indexY + lengthY; j++) {
      z = [...z, data[0].z[j].slice(indexX, indexX + lengthX).map(z => z + 0.5)];
    }

    let trace = {
      z: z,
      x: data[0].x.slice(indexX, indexX + lengthX),
      y: data[0].y.slice(indexY, indexY + lengthY),
      colorscale: [[0, "hsl(300,100,50)"], [1, "hsl(300,100,50)"]],
      type: 'surface',
      showscale: false,
      opacity: 0.5
    }
    if (z.length > 0) {
      //Plotly.purge(htmlelement);
      data.push(trace);
      const wy = 2 / (y_off_end - y_off);
      //const cy = (y_off + y_off_end) / (y_off - y_off_start); 

      const wx = 2 / (x_off_end - x_off);
      const cx = (x_off + x_off_end) / (x_off - x_off_end);

      const ax = v => (v * wx + cx) * (1 / layout.scene.aspectratio.y);
      const ay = v => (v - ((y_off_start + y_off_end) / 2)) * wy;// + cy;

      const magx = Math.max(width / (x_off_end - x_off), 0);
      const magy = Math.max(length / (y_off_end - y_off_start), 0);

      const mag = Math.max(magx, magy);

      layout.scene.camera.eye.x = defaultEyePosition.x + mag * ax(x + width / 2);
      layout.scene.camera.center.x = ax(x + width / 2);

      layout.scene.camera.eye.y = (defaultEyePosition.y + ay(y + length / 2));
      layout.scene.camera.center.y = ay(y + length / 2);

      layout.scene.camera.eye.z = mag * (defaultEyePosition.z);
      Plotly.react(htmlelement, data, layout);
    }

  }


  function update_plot_b(buf, y) {

    const plotData = plot_data.getRootAsplot_data(new ByteBuffer(new Uint8Array(buf)));
    const x_min = plotData.xOff() / 1000;
    const y_axis = plotData.ySamplesArray().map(y => y / 1000);
    const rows = plotData.ySamplesLength();
    const columns = plotData.zSamplesLength() / rows;
    const x_resolution = x_res / 1000; // convert mm to m
    const y_resolution = y_res / 1000;
    const belt_width = columns * x_resolution;
    const belt_length = y_axis[y_axis.length - 1] - y_axis[0];

    const z_surface = generate_ploty_z_samples(plotData.zSamplesArray(), columns);

    const x_axis = [...Array(columns).keys()].map(x => x_min + x * x_resolution);

    data[0].z = z_surface;
    data[0].x = x_axis;
    data[0].y = y_axis;

    layout.scene.yaxis.range = [y_axis[0], y_axis[y_axis.length - 1]];
    layout.scene.xaxis.range = [x_axis[0], x_axis[x_axis.length - 1]];
    layout.scene.aspectratio.y = belt_length / belt_width;
    layout.scene.aspectratio.z = z_max / (belt_width * 1000);
    layout.scene.camera.eye = { ...defaultEyePosition };
    layout.scene.camera.center = { x: 0, y: 0, z: 0 };

    // Pull edges to zero to mimic 3d belt
    for (let i = 0; i < rows; i++) {
      z_surface[i][0] = this.zMIn;
      z_surface[i][columns - 1] = this.zMin;
    }

    for (let i = 0; i < columns; i++) {
      z_surface[0][i] = this.zMin;
      z_surface[rows - 1][i] = this.zMin;
    }

    const bottom_plane = mk_2dArray(rows, columns,this.zMin);

    data[1].y = y_axis;
    data[1].x = x_axis;
    data[1].z = bottom_plane;

    //Plotly.purge(htmlelement);
    //(async _ => {await null; Plotly.update(htmlelement, trace, ulayout,0);})();
    data.splice(2);
    Plotly.react(htmlelement, data, layout, config);
    //Plotly.newPlot(htmlelement, data, layout);
    var myPlot = document.getElementById(htmlelement);

    myPlot.on('plotly_click', function (e) {
      const y = e.points[0].pointNumber[1];
      data2[0].y = data[0].z[y];
      data2[1].y = data2[0].y.map(y => 16.2 * (y > cut_y));
      Plotly.react(htmlelement + "-profile", data2, layout2, config);
    });


    layout2.xaxis = [x_axis[0], x_axis[x_axis.length - 1]];

    layout2.shapes[0].x0 = layout2.xaxis[0];
    layout2.shapes[0].x1 = layout2.xaxis[1];
    layout2.aspectratio.y = z_max / (belt_width * 1000);

    data2[0].y = z_surface[Math.floor(columns / 2)];
    data2[0].x = x_axis;
    data2[1].y = data2[0].y.map(y => 16.2 * (y > cut_y));
    data2[1].x = x_axis;
    Plotly.react(htmlelement + "-profile", data2, layout2, config);
    console.debug(`update_plot(${y}) finished`);
    //plot_overlay(700,50,0,50);
  }


  async function update_plot(belt, y, window_len) {

    console.debug(`update_plotb(${belt},${y},${window_len})`);
    await fetch(`api/belt/${belt}/${y}/${window_len}`)
      .then(r => {
        if (!r.ok) throw new Error();
        //return r.body.getReader();
        return r.arrayBuffer();
      })
      .then(r => update_plot_b(r, y))
      .catch();

  }


  //w.onmessage = (e) => {
  //  update_plot(Math.floor(num_plot_y_samples/2)*y_sample_len); // Zero is offset into buffer
  //}

  const data = [
    {
      // z: generate_ploty_z_samples(buffer,0,num_plot_y_samples,num_x_samples),
      colorscale: color_scale,
      //reversescale: !Array.isArray(color_scale),
      cmax: z_max,
      cmin: z_min,
      type: 'surface',
      contours: {
        x: { highlight: false }, y: { highlight: false },
        z: {
          highlight: true,
          show: true,
          //usecolormap: true,
          highlightcolor: "#42f462",
          project: { z: false }
        }
      },
      //y: generate_ploty_y_axis(0,num_plot_y_samples,num_y_samples,y_sample_len), //[...Array(num_plot_y_samples).keys()].map(x => x * y_sample_len),
      //x: [...Array(num_x_samples).keys()].map(x => x_start + x * belt_width / num_x_samples)
    }
  ];

  data.push
    (
      {
        //z: bottom_plane,
        cmax: z_max,
        cmin: z_min,
        colorscale: color_scale,
        showscale: false,
        //reversescale: !Array.isArray(color_scale),
        type: 'surface',
        //y: data[0].y,
        //x: data[0].x
      }
    );




  //const h = Plotly.react(htmlelement, data, layout, config);
  // update_plot("jimblebar","cv504",,256);


  var trace1 = {
    // x: [...Array(num_x_samples).keys()].map(x => x_start + x * belt_width / num_x_samples),
    // y: generate_ploty_z_samples(buffer,0,num_plot_y_samples,num_x_samples)[127],
    type: 'scatter',
    fill: 'tonexty',
    fillcolor: "#f77f00",
    showlegend: false,
    line: {
      color: "#f77f00"
    }
  };

  var trace2 = {
    // x: [...Array(num_x_samples).keys()].map(x => x_start + x * belt_width / num_x_samples),
    // y: [...Array(num_x_samples).keys()].map(x => 16.2),
    type: 'scatter',
    fill: 'tozeroy',
    fillcolor: '#ffd691',
    showlegend: false,
    line: {
      color: "#ffd691"
    }
  };

  var data2 = [trace1, trace2];


  handles.push({
    update_plot: update_plot,
    resize_plot: resize_plot,
    plot_overlay: plot_overlay,
    plot_change_colorscale: plot_change_colorscale
  });

  return handles.length - 1

}

function plot_change_colorscale(handle, c) {
  handles[handle].plot_change_colorscale(c)
}

function plot_overlay(handle, x, width, y, length) {
  handles[handle].plot_overlay(x, width, y, length);
}

async function update_plot(handle, belt, y, len) {
  await handles[handle].update_plot(belt, y, len);
}

function resize_plot(handle) {
  handles[handle].resize_plot();
}

function getBoundingClientRect(elementId) {
  const o = document.querySelector(elementId).getBoundingClientRect();
  return { x: o.left, y: o.top, right: o.right, bottom: o.bottom };
}


class PlotDataCache {
  constructor(blazor) {
    this.cacheCount = 0;
    this.cache = new Map();
    this.lastInsert = new Set();
    this.blazor = blazor;
  }


  compareSets(a, b) {
    let r = true;
    for (const v of a) {
      r = r && b.has(v);
    }

    return r;
  }

  async inCacheSet(c) {

    for (const k of this.cache.keys()) {
      if (this.compareSets(k, c)) {
        const p = this.cache.get(k);
        let resolved = false;
        await p.then(e => resolved = true);
        return [resolved, p];
      }
    }

    return [false, {}]
  }

  async inCache(belt, y, windowLen) {

    const c = new Set([belt, y, windowLen]);
    return await this.inCacheSet(c);
  }

  async insertCache(belt, y, windowLen, data) {

    const s = new Set([belt, y, windowLen]);

    if (this.cacheCount > 3) {
      this.cache.delete(this.cache.keys().next().value);
      this.cacheCount--;
    }

    this.cacheCount++;
    this.cache.set(s, data);
    this.lastInsert = s;
  }

  async getLastFetch() {
    const [_, data] = await this.inCacheSet(this.lastInsert);
    return data;
  }

  async deserialization(buf) {
    const plotDataBuf = plot_data.getRootAsplot_data(new ByteBuffer(new Uint8Array(buf)));
    const x_min = plotDataBuf.xOff();
    const y_axis = plotDataBuf.ySamplesArray();
    const z_surface = plotDataBuf.zSamplesArray();

    return { xMin: x_min, yAxis: y_axis, zSurface: z_surface }
  }

  async fetchData(belt, y, windowLen, leftLen) {

    const [inCache, data] = await this.inCache(belt, y, windowLen);

    if (inCache) {
      return data;
    }

    const blazor = this.blazor;
    const plotDataPromise = fetch(`api/belt/${belt}/${y}/${windowLen}/${leftLen}`)
      .then(r => {
        if (!r.ok) throw new Error(`Response not ok. ${r.statusText}`);
        const reader = r.body.getReader();

        const contentLength = r.headers.get('Content-Length');
        let progress = 0;

        return new ReadableStream({
          start(controller) {
            return pump();
            function pump() {
              return reader.read().then(({ done, value }) => {
                // When no more data needs to be consumed, close the stream
                
                if (done) {
                  controller.close();
                  return;
                }

                progress += value.length;
                blazor.invokeMethodAsync('SetProgressPercentageAsync', (progress / contentLength));
                controller.enqueue(value);
                return pump();
              });
            }
          }
        })
      })
      .then(stream => new Response(stream))
      .then(r => r.arrayBuffer())
      .then(buf => this.deserialization(buf))
      .catch();

    await this.insertCache(belt, y, windowLen, plotDataPromise);
    return plotDataPromise;
  }

}

class LinePlot {
  constructor(plotElement) {
    this.plotElement = plotElement;

    this.layout = {
      title: {
        text: "Wear Rate of Selected Point on Profile",
        font : {
          family : "Century Gothic",
          size : 24
        },
        x : 0.05
      },
      autosize: true,
      yaxis: {
        range: [12, 36],
        side: 'right'
      },

      xaxis: {},

      margin: {
        l: 40,
        r: 40,
        b: 40,
        t: 40,
      },

      aspectratio: {
        x: 1
      },

      hovermode : "closest"
    };

    this.config = {
      displaylogo: false,
      displayModeBar: true
    };


    const trace = {
      type: 'scatter',
      showlegend: false,
      line: {
        color: "#f77f00"
      }
    };

    this.plotData = [trace];
    
  }

  async updatePlot(x_axis,y_axis) {

    this.layout.xaxis = [x_axis[0], x_axis[x_axis.length - 1]];

    this.plotData[0].y = y_axis;
    this.plotData[0].x = x_axis;
    await Plotly.react(this.plotElement, this.plotData, this.layout, this.config);
  }


}


class TrendPlot {
  constructor(plotElement, x_res, z_min, z_max, blazor) {
    this.plotElement = plotElement;
    this.xRes = x_res;
    this.zMax = z_max;
    this.zMin = z_min;

    this.layout = {
      title: {
        text: "Belt Cross Section Profile",
        font : {
          family : "Century Gothic",
          size : 24
        },
      x : 0.05
    },
      autosize: true,
      yaxis: {
        range: [z_min, z_max],
        side: 'right'
      },

      xaxis: {},

      margin: {
        l: 40,
        r: 40,
        b: 40,
        t: 40,
      },

      aspectratio: {
        x: 1
      },

      hovermode : "closest"
    };

    this.config = {
      displaylogo: false,
      displayModeBar: true
    };


    const trace = {
      type: 'scatter',
      showlegend: false,
      line: {
        color: "#f77f00"
      }
    };

    this.plotData = [structuredClone(trace),structuredClone(trace),structuredClone(trace),structuredClone(trace)];
    Plotly.react(this.plotElement, [], this.layout, this.config);

    this.plotElement.on('plotly_click', function (e) {
      const xIndex = e.points[0].pointNumber;
      blazor.invokeMethodAsync('TrendPlotClicked', xIndex);
    });
  }

  async updatePlot(index,xMin,z_profile,color) {

    const x_min = xMin / 1000; //convert mm to m
    const x_resolution = this.xRes / 1000; // convert mm to m
    const belt_width = z_profile.length * x_resolution;

    const x_axis = [...Array(z_profile.length).keys()].map(x => x_min + x * x_resolution);

    this.layout.xaxis = [x_axis[0], x_axis[x_axis.length - 1]];

    this.layout.aspectratio.y = (this.zMax - this.zMin) / (belt_width * 1000);

    this.plotData[index].y = z_profile;
    this.plotData[index].x = x_axis;
    this.plotData[index].line.color = color
    await Plotly.react(this.plotElement, this.plotData, this.layout, this.config);
  }


}

class ProfilePlot {
  constructor(plotElement, x_res, z_min, z_max) {
    this.plotElement = plotElement;
    this.xRes = x_res;
    this.zMax = z_max;
    this.zMin = z_min;

    this.layout = {
      autosize: true,
      yaxis: {
        range: [z_min, z_max],
        side: 'right'
      },

      xaxis: {},

      shapes: [{
        type: 'line',
        y0: 29.5,
        y1: 29.5,
        line: {
          color: 'rgb(50, 171, 96)',
          width: 2
        }

      }],

      margin: {
        l: 20,
        r: 20,
        b: 20,
        t: 20,
      },

      aspectratio: {
        x: 1
      }
    };

    this.config = {
      displaylogo: false,
      displayModeBar: false
    };


    const trace1 = {
      type: 'scatter',
      fill: 'tonexty',
      fillcolor: "#f77f00",
      showlegend: false,
      line: {
        color: "#f77f00"
      }
    };

    const trace2 = {
      type: 'scatter',
      fill: 'tozeroy',
      fillcolor: '#ffd691',
      showlegend: false,
      line: {
        color: "#ffd691"
      }
    };

    this.plotData = [trace1, trace2];
  }

  async updatePlot(plotDataPromise, yIndex) {

    this.plotDataPromiseTop = plotDataPromise;
    const plotData = await plotDataPromise;

    yIndex = (yIndex || 0);

    const x_min = plotData.xMin / 1000; //convert mm to m
    const rows = plotData.yAxis.length;
    const columns = plotData.zSurface.length / rows;
    const x_resolution = this.xRes / 1000; // convert mm to m
    const belt_width = columns * x_resolution;

    const z_surface = generate_ploty_z_samples(plotData.zSurface, columns);

    const x_axis = [...Array(columns).keys()].map(x => x_min + x * x_resolution);

    this.layout.xaxis = [x_axis[0], x_axis[x_axis.length - 1]];

    this.layout.shapes[0].x0 = this.layout.xaxis[0];
    this.layout.shapes[0].x1 = this.layout.xaxis[1];
    this.layout.aspectratio.y = (this.zMax - this.zMin)  / (belt_width * 1000);

    this.plotData[0].y = z_surface[yIndex];
    this.plotData[0].x = x_axis;
    this.plotData[1].y = this.plotData[0].y.map(y => 16.2 * (y > 9.0));
    this.plotData[1].x = x_axis;
    await Plotly.react(this.plotElement, this.plotData, this.layout, this.config);
  }

  async updateY(yIndex) {
    if(this.plotDataPromiseTop && this.plotDataPromiseBottom) {
      await this.updatePlotDoubleSided(this.plotDataPromiseTop, this.plotDataPromiseBottom, yIndex);
    }else if(this.plotDataPromiseTop) {
      await this.updatePlot(this.plotDataPromiseTop, yIndex);
    }
  }
  
  async updatePlotDoubleSided(plotDataPromiseTop, plotDataPromiseBottom, yIndex) {

    this.plotDataPromiseTop = plotDataPromiseTop;
    this.plotDataPromiseBottom = plotDataPromiseBottom;

    const plotDataTop = await plotDataPromiseTop;
    const plotDataBottom = await plotDataPromiseBottom;

    yIndex = (yIndex || 0);

    const x_min = plotDataTop.xMin / 1000; //convert mm to m
    const rows = plotDataTop.yAxis.length;
    const columns = plotDataTop.zSurface.length / rows;
    const x_resolution = this.xRes / 1000; // convert mm to m
    const belt_width = columns * x_resolution;

    const z_surfaceTop = generate_ploty_z_samples(plotDataTop.zSurface, columns);
    const z_surfaceBottom = generate_ploty_z_samples(plotDataBottom.zSurface, columns);

    const x_axis = [...Array(columns).keys()].map(x => x_min + x * x_resolution);

    this.layout.xaxis = [x_axis[0], x_axis[x_axis.length - 1]];

    this.layout.shapes[0].x0 = this.layout.xaxis[0];
    this.layout.shapes[0].x1 = this.layout.xaxis[1];
    this.layout.aspectratio.y = (this.zMax - this.zMin)  / (belt_width * 1000);

    this.plotData[0].y = z_surfaceTop[yIndex];
    this.plotData[0].x = x_axis;
    this.plotData[1].y = z_surfaceBottom[yIndex];
    this.plotData[1].x = x_axis;
    this.plotData[1].fillcolor = '#ffffff';
    this.plotData[1].line.color = '#ffffff';
    await Plotly.react(this.plotElement, this.plotData, this.layout, this.config);
  }


}


class SurfacePlot {

  static get defaultEyePosition() {
    return Object.freeze({ x: 0.5, y: 0, z: 1.5 });
  }

  static get surfacePlotData() {
    return 0;
  }

  static get floorPlotData() {
    return 1;
  }

  static get overlayPlotData() {
    return 2;
  }


  constructor(plotElement, x_res, z_min, z_max, color_scale, blazor) {
    this.plotElement = plotElement;
    this.xRes = x_res;
    this.zMax = z_max;
    this.zMin = z_min;
    this.colorScale = typeof (color_scale) == 'string' ? color_scale : color_scale.map(({ item1, item2 }) => [item1, item2]);

    this.layout = {
      autosize: true,
      margin: {
        l: 0,
        r: 0,
        b: 0,
        t: 0,
      },
      scene: {
        camera: {
          center: {
            x: 0,
            y: 0,
            z: 0
          },
          eye: { ...SurfacePlot.defaultEyePosition }
        },

        zaxis: {
          range: [z_min, z_max]
        },

        yaxis: {
          showspikes: true,
          spikecolor: "magenta",
          spikemode: "across",
          spikethickness: 5,
        },

        xaxis: {
          showspikes: true,
          spikecolor: "magenta",
          spikethickness: 5,
          spikemode: "across",
        },

        aspectratio: {
          x: 1
        }
      } // end scene
    };


    this.config = {
      displaylogo: false,
      displayModeBar: true,
      modeBarButtonsToAdd: [
        {
          name: 'Reset camera to default',
          icon: Plotly.Icons.home,
          direction: 'up',
          surfacePlot : this,
          click: function (gd) {
            this.surfacePlot.layout.scene.camera.eye = { ...SurfacePlot.defaultEyePosition };
            this.surfacePlot.layout.scene.camera.center = { x: 0, y: 0, z: 0 };
            Plotly.relayout(gd, this.surfacePlot.layout);
          }
        }],

      modeBarButtonsToRemove: ['resetCameraDefault3d', 'resetCameraLastSave3d']
    };

    this.plotData = [
      {
        x: [],
        y: [],
        z: [],
        colorscale: this.colorScale,
        cmax: z_max,
        cmin: z_min,
        type: 'surface',
        contours: {
          x: { highlight: false },
          y: { highlight: false },
          z: {
            highlight: true,
            show: true,
            highlightcolor: "#42f462",
            project: { z: false }
          }
        },
        colorbar: {
          thickness: 15,
          len: 0.75
        }
      },
      {
        x: [],
        y: [],
        z: [],
        cmax: z_max,
        cmin: z_min,
        colorscale: this.colorScale,
        showscale: false,
        type: 'surface'
      },
      {
        x: [],
        y: [],
        z: [],
        colorscale: [[0, "hsl(300,100,50)"], [1, "hsl(300,100,50)"]],
        type: 'surface',
        showscale: false,
        opacity: 0.5
      }
    ];

    Plotly.react(this.plotElement, [], this.layout, this.config);

    this.plotElement.on('plotly_click', function (e) {
      const yIndex = e.points[0].pointNumber[1];
      const y = e.points[0].y * 1000;
      blazor.invokeMethodAsync('PlotClicked', yIndex,y);
    });

  }

  async updatePlotDataDoubleSided(plotDataPromiseTop,plotDataPromiseBottom) {

    const plotDataTop = await plotDataPromiseTop;
    const plotDataBottom = await plotDataPromiseBottom;

    const x_min = plotDataTop.xMin / 1000; //mm to m
    const y_axis = plotDataTop.yAxis.map( y => y / 1000); //mm to m
    const rows = plotDataTop.yAxis.length;
    let columns = plotDataTop.zSurface.length / rows;
    const x_resolution = this.xRes / 1000; // convert mm to m
    const belt_width = columns * x_resolution;
    const belt_length = y_axis[y_axis.length - 1] - y_axis[0];

    if(Math.floor(columns) !== columns) {
      console.error("Z samples not a multiple of y samples");
      columns = Math.floor(columns);
    }

    const z_surfaceTop = generate_ploty_z_samples(plotDataTop.zSurface, columns);

    const x_axis = [...Array(columns).keys()].map(x => x_min + x * x_resolution);

    this.plotData[SurfacePlot.surfacePlotData].z = z_surfaceTop;
    this.plotData[SurfacePlot.surfacePlotData].x = x_axis;
    this.plotData[SurfacePlot.surfacePlotData].y = y_axis;

    this.layout.scene.yaxis.range = [y_axis[0], y_axis[y_axis.length - 1]];
    this.layout.scene.xaxis.range = [x_axis[0], x_axis[x_axis.length - 1]];
    this.layout.scene.aspectratio.y = belt_length / belt_width;
    this.layout.scene.aspectratio.z = (this.zMax - this.zMin)  / (belt_width * 1000);
    this.layout.scene.camera.eye = { ...SurfacePlot.defaultEyePosition };
    this.layout.scene.camera.center = { x: 0, y: 0, z: 0 };

    const bottom_plane = generate_ploty_z_samples(plotDataBottom.zSurface, columns);
    mk_BottomSurface(z_surfaceTop,bottom_plane);

    // Pull edges to zero to mimic 3d belt
    for (let i = 0; i < rows; i++) {
      z_surfaceTop[i][0] = bottom_plane[i][0];
      z_surfaceTop[i][columns - 1] = bottom_plane[i][columns - 1];
    }

    for (let i = 0; i < columns; i++) {
      z_surfaceTop[0][i] = bottom_plane[0][i];
      z_surfaceTop[rows - 1][i] = bottom_plane[rows - 1][i];
    }



    this.plotData[SurfacePlot.floorPlotData].y = y_axis;
    this.plotData[SurfacePlot.floorPlotData].x = x_axis;
    this.plotData[SurfacePlot.floorPlotData].z = bottom_plane;

    this.clearOverlay();
    
    return y_axis[y_axis.length - 1];
  }

  async updatePlotData(plotDataPromise) {

    const plotData = await plotDataPromise;
    const x_min = plotData.xMin / 1000; //mm to m
    const y_axis = plotData.yAxis.map( y => y / 1000); //mm to m
    const rows = plotData.yAxis.length;
    let columns = plotData.zSurface.length / rows;
    const x_resolution = this.xRes / 1000; // convert mm to m
    const belt_width = columns * x_resolution;
    const belt_length = y_axis[y_axis.length - 1] - y_axis[0];

    if(Math.floor(columns) !== columns) {
      console.error("Z samples not a multiple of y samples");
      columns = Math.floor(columns);
    }

    const z_surface = generate_ploty_z_samples(plotData.zSurface, columns);

    const x_axis = [...Array(columns).keys()].map(x => x_min + x * x_resolution);

    this.plotData[SurfacePlot.surfacePlotData].z = z_surface;
    this.plotData[SurfacePlot.surfacePlotData].x = x_axis;
    this.plotData[SurfacePlot.surfacePlotData].y = y_axis;

    this.layout.scene.yaxis.range = [y_axis[0], y_axis[y_axis.length - 1]];
    this.layout.scene.xaxis.range = [x_axis[0], x_axis[x_axis.length - 1]];
    this.layout.scene.aspectratio.y = belt_length / belt_width;
    this.layout.scene.aspectratio.z = (this.zMax - this.zMin) / (belt_width * 1000);
    this.layout.scene.camera.eye = { ...SurfacePlot.defaultEyePosition };
    this.layout.scene.camera.center = { x: 0, y: 0, z: 0 };

    // Pull edges to zero to mimic 3d belt
    for (let i = 0; i < rows; i++) {
      z_surface[i][0] = this.zMin;
      z_surface[i][columns - 1] = this.zMin;
    }

    for (let i = 0; i < columns; i++) {
      z_surface[0][i] = this.zMin;
      z_surface[rows - 1][i] = this.zMin;
    }

    const bottom_plane = mk_2dArray(rows, columns,this.zMin);

    this.plotData[SurfacePlot.floorPlotData].y = y_axis;
    this.plotData[SurfacePlot.floorPlotData].x = x_axis;
    this.plotData[SurfacePlot.floorPlotData].z = bottom_plane;

    this.clearOverlay();
    
    return y_axis[y_axis.length - 1];
  }

  clearOverlay() {
    
    this.plotData[SurfacePlot.overlayPlotData].y = [];
    this.plotData[SurfacePlot.overlayPlotData].x = [];
    this.plotData[SurfacePlot.overlayPlotData].z = [];

  }

  async updatePlot(plotDataPromise,xRes) {
    this.xRes = xRes;
    const y = await this.updatePlotData(plotDataPromise);
    await this.generatePlot();
    return y;
  }

  async updatePlotDoubleSided(plotDataPromiseTop,plotDataPromiseBottom,xRes) {
    this.xRes = xRes;
    const y = await this.updatePlotDataDoubleSided(plotDataPromiseTop,plotDataPromiseBottom);
    await this.generatePlot();
    return y;
  }

  async updatePlotWithOverlay(plotDataPromise,zDepth) {
    const y = await this.updatePlotData(plotDataPromise);
    await this.addRectOverlay(zDepth);
    await this.generatePlot();
    return y;
  }

  async generatePlot() {
    Plotly.react(this.plotElement, this.plotData, this.layout, this.config);
  }

  async addRectOverlay(zDepth) {
    console.debug(`addRectOverlay ${zDepth}`);

    const xLength = this.plotData[SurfacePlot.surfacePlotData].x.length;
    const xArray = this.plotData[SurfacePlot.surfacePlotData].x;
    const yLength = this.plotData[SurfacePlot.surfacePlotData].y.length;
    const yArray = this.plotData[SurfacePlot.surfacePlotData].y;
    
    const xyRatio = ((yArray[yLength - 1] - yArray[0]) / yLength) / ((xArray[xLength - 1] - xArray[0]) / xLength);
    const width = 50; // x axis
    const length = Math.floor(width / xyRatio); // y axis

    const yIndex = this.plotData[SurfacePlot.surfacePlotData].y.findIndex( y => zDepth.z.y <= y*1000);
    const xIndex = this.plotData[SurfacePlot.surfacePlotData].x.findIndex( x => zDepth.z.x <= x*1000);

    const yMin = Math.max(yIndex - length,0);
    const xMin = Math.max(xIndex - width,0);

    const Y = this.plotData[SurfacePlot.surfacePlotData].y.slice(yMin,yIndex + length);
    const X = this.plotData[SurfacePlot.surfacePlotData].x.slice(xMin,xIndex + width);
    const Z = this.plotData[SurfacePlot.surfacePlotData].z.slice(yMin,yIndex + length).map( x=> x.slice(xMin,xIndex + width)).map(x => x.map( z => z+0.5)); 
    this.plotData[SurfacePlot.overlayPlotData].y = Y;
    this.plotData[SurfacePlot.overlayPlotData].x = X;
    this.plotData[SurfacePlot.overlayPlotData].z = Z;

  }

  changeColorScale(c) {
    c = c.map(({ item1, item2 }) => [item1, item2]);

    this.plotData[SurfacePlot.surfacePlotData].colorscale = c;
    this.plotData[SurfacePlot.floorPlotData].colorscale = c;
    Plotly.react(this.plotElement, this.plotData, this.layout);
  }

}

export function mk_SurfacePlot(plotElement, x_res, z_min, z_max, color_scale, blazor) {
  return new SurfacePlot(plotElement, x_res, z_min, z_max, color_scale, blazor);
}

export function mk_ProfilePlot(plotElement, x_res, z_min, z_max) {
  return new ProfilePlot(plotElement, x_res, z_min, z_max);
}

export function mk_TrendPlot(plotElement, x_res, z_min, z_max,blazor) {
  return new TrendPlot(plotElement, x_res, z_min, z_max,blazor);
}

export function mk_LinePlot(plotElement) {
  return new LinePlot(plotElement);
}

export function mk_PlotDataCache(...args) {
  return new PlotDataCache(...args);
}


