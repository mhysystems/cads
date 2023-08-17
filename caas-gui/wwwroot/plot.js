import { profile_array, ByteBuffer } from './cads-flatbuffers-plot.es.js'

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

function array16ToFloat(array16,zResolution,zOffset)
{
  const f32 = new Float32Array(array16.length);
  for(let i = 0; i < array16.length; ++i) {
    const e = array16[i];
    f32[i] =  e != -32768 ? (e * zResolution + zOffset) : NaN;
  }

  return f32;
}

class CanvasPlot {
  constructor(canvas,noiseValueElement) {
    this.ctx = canvas.getContext("2d");
    this.noiseValueElement = noiseValueElement;

  }

  async updatePlot(bytes) {

    const profileDataBuf = profile_array.getRootAsprofile_array(new ByteBuffer(new Uint8Array(bytes,bytes.byteOffset,bytes.byteLength)));
    const z_resolutiom = profileDataBuf.zRes();
    const z_Offset = profileDataBuf.zOff();
    const profile = profileDataBuf.profiles(0);
    const z = array16ToFloat(profile.zSamplesArray(),z_resolutiom,z_Offset);
    const nanCnt = Math.floor(100*profile.xOff());
    // Need to slice buffer as byteOffset is sometimes not a multiple of Float32 needed for memeory alignment.
    
    if(z.byteLength < Float32Array.BYTES_PER_ELEMENT) 
    {
      return;
    }

    const noise = nanCnt.toLocaleString(undefined, { maximumFractionDigits: 0, minimumFractionDigits: 0 });
    this.noiseValueElement.textContent = noise;

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

export function mk_CanvasPlot(canvas,noiseValueElement) {
  return new CanvasPlot(canvas,noiseValueElement);
}