import { Msg, CaasProfile, MsgContents, ByteBuffer } from './cads-flatbuffers-plot.es.js'

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
  constructor(canvas,noiseValueElement) {
    this.ctx = canvas.getContext("2d");
    this.noiseValueElement = noiseValueElement;

  }

  async clearPlot() {
    this.ctx.reset();
    this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    this.ctx.beginPath();
  }

  async updatePlot(bytes) {
    
    const cheight = this.ctx.canvas.height;
    const cwidth = this.ctx.canvas.width;

    const msgDataBuf = Msg.getRootAsMsg(new ByteBuffer(new Uint8Array(bytes,bytes.byteOffset,bytes.byteLength)));
    const msgType = msgDataBuf.contentsType();

    if(msgType != MsgContents.CaasProfile) {
      console.error("msg not a caasProfile type")
      return;
    }

    const profile = msgDataBuf.contents(new CaasProfile());

    const z = profile.zSamplesArray();
    const nanCnt = Math.floor(100*profile.nanRatio());
    
    if(z.byteLength < Float32Array.BYTES_PER_ELEMENT) 
    {
      return;
    }

    const noise = nanCnt.toLocaleString(undefined, { maximumFractionDigits: 0, minimumFractionDigits: 0 });
    
    const xmap = (i) => {
      const s = i * profile.xResolution() + profile.xOff();
      return Math.floor((s - profile.xOrigin()) * (cwidth / profile.width()));
    };

    const zmap = (z) => {
      return Math.floor((z - profile.zOrigin()) * (cheight / profile.height()));
    };
    
    this.noiseValueElement.textContent = noise;

    const [zmin,zmax] = minmax(z);

    //interpolatetoN(z,cwidth);

    this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    this.ctx.beginPath();
    this.ctx.moveTo(0, cheight);
    this.ctx.lineTo(cwidth / 2, 0);
    this.ctx.lineTo(cwidth, cheight);
    this.ctx.clip();

    const grd = this.ctx.createLinearGradient(0, 0, 0, cheight);
    grd.addColorStop(0, "red");
    grd.addColorStop(1 - (250 / profile.height()), "white");
    this.ctx.fillStyle = grd;
    this.ctx.fillRect(0, 0, cwidth, cheight);


    this.ctx.fillStyle = "red";
    for(let x = 0 ; x < z.length; ++x) {
      let i = xmap(x);
      if(i < cwidth && !isNaN(z[x])) {
        let dbg = zmap(z[x]);
        this.ctx.fillRect(i,cheight - dbg,2,2);
      }
    }

    
    this.ctx.lineWidth = 1;
    this.ctx.strokeStyle = "green";
    this.ctx.beginPath();
    this.ctx.moveTo(0,cheight - zmap(profile.zOrigin()+250));
    this.ctx.lineTo(cwidth,cheight - zmap(profile.zOrigin()+250));
    this.ctx.stroke();
    
  }
}

export function mk_CanvasPlot(canvas,noiseValueElement) {
  return new CanvasPlot(canvas,noiseValueElement);
}