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
  const narray = new Array(N);

  for(let i = 0; i < N; i++) {
    narray[i] = array[Math.floor(i*step)];
  }

  return narray;
}

class CanvasPlot {
  constructor(canvas) {
    this.ctx = canvas.getContext("2d");
  }

  async updatePlot(bytes) {
    // Need to slice buffer as byteOffset is sometimes not a multiple of Float32 needed for memeory alignment.
    const z = new Float32Array(bytes.buffer.slice(bytes.byteOffset),0,bytes.byteLength / Float32Array.BYTES_PER_ELEMENT); 
    
    if(z.byteLength < Float32Array.BYTES_PER_ELEMENT) 
    {
      return;
    }
    
    const [zmin,zmax] = minmax(z);
    const cheight = this.ctx.canvas.height;
    const cwidth = this.ctx.canvas.width;
    interpolatetoN(z,cwidth);

    const ztoy = (zmax - zmin) < 60 ? (z) => {
      const scale = cheight / ((zmax - zmin) * 2);

      return 0.75*cheight - (z - zmin)*scale;
    } : (z) => {
      const scale = cheight / (zmax - zmin);

      return cheight - (z - zmin)*scale;
    }

    this.ctx.reset();
    this.ctx.fillStyle = "red";
    for(let x = 0 ; x < cwidth; ++x) {
      if(!isNaN(z[x])) {
        let dbg = Math.floor(ztoy(z[x]));
        this.ctx.fillRect(x,dbg,2,2);
      }
    }

    this.ctx.lineWidth = 1;
    this.ctx.strokeStyle = "white";
    this.ctx.beginPath();
    for(const off of [0.25,0.75]) {
      this.ctx.moveTo(0,Math.floor(cheight*off));
      this.ctx.lineTo(cwidth,Math.floor(cheight*off));
    }
    this.ctx.stroke();
  }
}

export function mk_CanvasPlot(canvas) {
  return new CanvasPlot(canvas);
}