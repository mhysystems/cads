function minmax(array) {

  let min = array[0];
  let max = array[0];

  for(let i=0; i < array.length; i++) {
    if(array[i] < min) {
      min = array[i];
    }
    if(array[i] > max) {
      max = array[i];
    }
  }

  return [min,max];
}

function interpolatetoN(array,N)
{
  const step = array.length / N;

  console.assert(array.length > N);
  console.assert(step > 0);

  for(let i = 0; i < N; i++) {
    array[i] = array[Math.floor(i*step)];
  }

  return array;
}

class CanvasPlot {
  constructor(canvas) {
    this.ctx = canvas.getContext("2d");
  }

  async updatePlot(bytes) {
    // Need to slice buffer as byteOffset is sometimes not a multiple of Float32 needed for memeory alignment.
    const z = new Float32Array(bytes.buffer.slice(bytes.byteOffset),0,bytes.byteLength / Float32Array.BYTES_PER_ELEMENT); 
    const [zmin,zmax] = minmax(z);
    const cheight = this.ctx.canvas.height;
    const cwidth = this.ctx.canvas.width;
    interpolatetoN(z,cwidth);

    function ztoy(z) {
      const scale = cheight / (zmax - zmin);

      return cheight - (z - zmin)*scale;
    }

    this.ctx.reset();
    for(let x = 0 ; x < cwidth; ++x) {
      if(!isNaN(z[x])) {
        let dbg = Math.floor(ztoy(z[x]));
        this.ctx.fillRect(x,dbg,2,2);
      }
    }
  }
}

export function mk_CanvasPlot(canvas) {
  return new CanvasPlot(canvas);
}