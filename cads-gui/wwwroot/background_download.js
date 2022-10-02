"use strict";

function filter_until_found(search_str) {

  return new TransformStream({
    start() { this.found = false },
    transform(str, controller) {

      if (this.found) {
        controller.enqueue(str);
        return;
      }

      const i = str.indexOf(search_str);
      if(i >= 0) {
        this.found = true;
        const dbg = str.substr(i + search_str.length)
        controller.enqueue(dbg);
      }
    }
  })

}


function split(split_str) {

  return new TransformStream({
    start() { this.store = "" },
    transform(str, controller) {

      const t = (this.store + str).split(split_str);
      this.store = t.splice(-1);
      t.forEach(e => controller.enqueue(e));
    }
  })

}

function stream_to_arraybuffer(buffer,num_plot_y_samples) {
  return  new WritableStream({
    start() { this.buffer_offset = 0 },
    
    write(elem) {
      return new Promise((resolve, reject) => {

        
        const f = new Float32Array(buffer, this.buffer_offset);
        //const t = elem.split(',').slice(1).map(a => a ? parseFloat(a) : 0.0);
        const t = elem.split(',').map(a => a ? parseFloat(a)*(29.6/59.3) : 0.0);

        f.set(t);

        if (this.buffer_offset === t.length * Float32Array.BYTES_PER_ELEMENT * num_plot_y_samples) {
          postMessage({}); //Signal enough data has been downloaded to plot one frame. 
        }

        this.buffer_offset += t.length * Float32Array.BYTES_PER_ELEMENT;

        resolve();
      });
    },
    close() {
      console.log("close");
    },
    abort(err) {
      console.log("Sink error:", err);
    }
  });

}

async function load_buffer(buffer, url, num_plot_y_samples) {
  const res = await fetch(url);
  
  if (res.ok) {
    
    const file = (await res.body);
    
    file.pipeThrough(new TextDecoderStream())
      //.pipeThrough(filter_until_found('Frame'))
      //.pipeThrough(filter_until_found('Frame'))
      .pipeThrough(filter_until_found('\n')) // Skip header
      .pipeThrough(split('\n'))
      .pipeTo(stream_to_arraybuffer(buffer,num_plot_y_samples));

  } else {
    console.log(res);
    return Promise.reject(res);
  }

}

function generate_ploty_z_samples(buffer,y_sample_start,num_y_samples,num_x_samples) {
  
  const mod = (x,y) => ((x % y) + y) % y;
  
  const z_samples = [];
  //const buffer_offset =  y_sample_start * num_x_samples * Float32Array.BYTES_PER_ELEMENT;
  const buffer_offset =  0;

  for (let i = 0; i < num_y_samples; i++) {
    z_samples.push(new Float32Array(buffer, mod(buffer_offset + i*num_x_samples*Float32Array.BYTES_PER_ELEMENT, buffer.byteLength), num_x_samples));
  }

  return z_samples;
}

function generate_ploty_y_axis(y_sample_start,num_plot_y_samples,num_y_samples,y_sample_len) {
    
  const mod = (x,y) => x; //(x,y) => ((x % y) + y) % y;

  const dbg =  [...Array(num_plot_y_samples).keys()].map(x => mod(y_sample_start + x,num_y_samples) * y_sample_len);
  console.log(`yaxis range: ${dbg[0]},${dbg[dbg.length -1]} length: ${dbg.length }`)

  return dbg;

}  



function update_plot(y,c,c_min,c_max) {
  c = (c &&( typeof(c) === 'string' ? c : c.map(({item1,item2}) => [item1,item2])));
  console.debug(`update_plot(${y},${c},${c_min},${c_max})`);


  const seg = y; //Math.floor(y / y_sample_len );
  data[0].z = generate_ploty_z_samples(buffer,seg,num_plot_y_samples,num_x_samples,num_plot_y_samples);
  c && (data[0].colorscale = c) ;
  if(c_min) data[0].cmin = c_min;
  if(c_max) data[0].cmax = c_max;

  c && (data[1].colorscale = c) ;
  if(c_min) data[1].cmin = c_min;
  if(c_max) data[1].cmax = c_max;
  for(let i = 0; i < num_x_samples; i++) {
    data[0].z[0][i] = z_min;
    data[0].z[num_plot_y_samples-1][i] = z_min;
  }

  for(let i = 0; i < num_plot_y_samples; i++) {
    data[0].z[i][0] = z_min;
    data[0].z[i][num_x_samples - 1] = z_min;
  }
  console.log(`z dim(Y,X): ${data[0].z.length},${data[0].z[0].length}`);
  data[0].y = generate_ploty_y_axis(seg,num_plot_y_samples,num_y_samples,y_sample_len);// [...Array(num_plot_y_samples).keys()].map(x => (seg + x) * y_sample_len);
  data[1].y = data[0].y;
  (async _ => {await null; Plotly.update(htmlelement, data, layout);})();
 
}


onmessage = (e) => {

  const buffer = e.data.b;
  const url = e.data.f;
  const num_plot_y_samples = e.data.s;

  load_buffer(buffer,url,num_plot_y_samples);

}