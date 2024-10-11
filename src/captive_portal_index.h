#include <pgmspace.h>
// captive_portal_index.h built at 2024-10-11T09:37:30.025Z
#ifndef __CAPTIVE_PORTAL_INDEX
#define __CAPTIVE_PORTAL_INDEX

const char index_html[] PROGMEM = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>The Attention Button - Setup</title>
<style>#cf-alert{width:40%;background:#fff;border-radius:.2rem;max-height:100%;overflow:auto}#alert-footer,#alert-header{padding:1rem}#alert-body>input{display:block;margin-top:1rem;margin-bottom:1rem}#alert-body{padding:1rem}#alert-body fieldset{border:none}#alert-body textarea{height:3rem;resize:none;margin-top:.5rem;font-family:inherit;display:block;width:calc(100% - .5rem)}#alert-footer{text-align:right}#alert-header{font-weight:700}#alert-ok{color:#fff;background-color:#007cdf;border:none;padding:.5rem;min-width:10ch;border-radius:.2rem}#alert-cancel{color:#fff;background-color:#978e0f;border:none;padding:.5rem;min-width:10ch;border-radius:.2rem}#alert-cancel,#alert-ok{cursor:pointer;margin:.2rem}@media not all and (hover:none){#alert-cancel:hover,#alert-ok:hover{filter:brightness(1.2)}}@media only screen and (max-aspect-ratio:1/1){#cf-alert{width:70%}}@media only screen and (max-aspect-ratio:9/16){#cf-alert{width:90%}}html{background-color:pink;width:100%;height:100%;color:#303030;display:flex;align-items:center;flex-direction:column;margin:0;padding:0}h1{margin-bottom:1rem}body{padding:1rem;padding-top:5rem;max-width:80ch;margin:0;width:calc(100% - 2rem);min-height:calc(100vh - 5rem);font-family:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Oxygen,Ubuntu,Cantarell,'Open Sans','Helvetica Neue',sans-serif}html{padding-bottom:3rem}details{border:2px solid;padding:1rem;margin-top:1rem;margin-bottom:1rem;background-color:#f5f4f0}details[open] summary{margin-bottom:1rem}summary{cursor:pointer}input[type=text][id^=secret-token]{max-width:12ch}input[type=text]:active{border-bottom-color:#303030}button,input[type=password],input[type=text]{margin:.4rem}button{min-width:10ch;padding:.4rem;background-color:#fff;border:2px solid gray}button:active{border:2px solid #000}input[type=password],input[type=text]{background-color:transparent;border:none;border-bottom:2px solid gray;padding:.2rem;font-size:1rem}.hidden{display:none}aside{margin:.5rem;color:#444;text-align:center}#secret-container{margin-bottom:1rem;display:flex;justify-content:center}#secret-controls{text-align:center}#wifi-form{max-width:40ch;width:100%;display:flex;flex-direction:column}.form-group{margin:.4rem;align-items:center;display:flex;justify-content:space-between}.submit-group{margin-top:.7rem;display:flex;justify-content:center}.wifi-form-wrapper{display:flex;align-items:center;flex-direction:column}.networks-container-label{text-align:center;margin-top:.5rem;margin-bottom:.5rem;font-weight:700}.networks-container>aside:last-child{text-align:left}.networks{display:grid;grid-template-columns:1fr 1fr 1fr;place-items:center;width:100%;margin:.4rem;gap:1rem}.network{padding:.5rem;border-radius:.4rem;background:#fff}.network header{margin-bottom:.4rem}#username-group{justify-content:center;align-items:center}.network-meta{display:flex;margin-top:.4rem}.network-meta>div{width:50%;text-align:left}.network{max-width:40ch}@media screen and (max-aspect-ratio:1/1){#secret-container{flex-direction:column;justify-content:unset;align-items:center}.form-group{flex-direction:column;align-items:flex-start;align-self:center;gap:.4rem}.form-group input{margin-left:0;margin-right:0}.networks{grid-template-columns:1fr 1fr}}@media screen and (max-aspect-ratio:9/14){.networks{grid-template-columns:1fr}}@media not all and (hover:none){button:hover{border-color:#404040;cursor:pointer}}</style>
</head>
<body>
<h1>The Attention Button - settings</h1>
<p>
Set your room secret and Wi-Fi settings here to pair The Attention Button with your partner's.
</p>
<details id="secret-menu" open aria-role="form">
<summary>Set room secret</summary>
<div id="username-group" class="form-group">
<label for="username">Your theattentionbutton.in username</label>
<input type="text" id="username">
</div>
<aside>Tip: paste the entire secret phrase into any field.</aside>
<div id="secret-container">
<label for="secret-token-first" class="hidden">Secret Token first part</label>
<input placeholder="First word" type="text" id="secret-token-first">
<label for="secret-token-second" class="hidden">Secret Token second part</label>
<input placeholder="Second word" type="text" id="secret-token-second">
<label for="secret-token-third" class="hidden">Secret Token third part</label>
<input placeholder="Third word" type="text" id="secret-token-third">
<label for="secret-token-fourth" class="hidden">Secret Token fourth part</label>
<input placeholder="Fourth word" type="text" id="secret-token-fourth">
</div>
<div id="secret-controls">
<button type="button" id="secret-token-submit">Set</button>
</div>
</details>
<details id="wifi-menu" open>
<summary>
Wi-Fi settings
</summary>
<div class="wifi-form-wrapper" aria-role="form">
<aside>Enter your Wi-Fi network's credentials. Don't worry, these never leave the device.</aside>
<div class="networks-container">
<div class="networks-container-label">Available networks</div>
<section class="networks">
</section>
<aside>If your network doesn't show up in the list, try restarting the device or moving it closer to your router.</aside>
</div>
<div aria-role="form" id="wifi-form">
<div class="form-group">
<label for="wifi-ssid">Network name</label>
<input type="text" id="wifi-ssid">
</div>
<div class="form-group">
<label for="wifi-psk">Network password</label>
<input type="password" id="wifi-psk">
</div>
<div class="submit-group">
<button id="wifi-submit" type="button" aria-role="submit">Set</button>
</div>
</div>
</div>
</details>
<script>(()=>{var k=(e,...t)=>{let s=[];for(let r=0;r<e.length;r++){s.push(e[r]||"");let i=t[r];typeof i<"u"&&typeof i!="object"?s.push(j((i||"").toString())):s.push(i?.contents||"")}return s.join("")},A=e=>{var t;let s=e?e.match(/([0-9a-zA-Z\-]*)?(#[0-9a-zA-Z\-]*)?((.[0-9a-zA-Z\-]+)*)/):void 0,r=s?(t=s.slice(1,4))===null||t===void 0?void 0:t.map(i=>i?i.trim():void 0):Array(3).fill(void 0);return r&&r[1]&&(r[1]=r[1].replace(/#*/g,"")),s?{tag:r[0]||void 0,id:r[1]||void 0,classes:r[2]?r[2].split(".").filter(i=>i.trim()):void 0}:{}},M=(e,t={})=>{let{contents:s,c:r,misc:i,m:c,style:l,s:v,on:d,attrs:n,a,raw:o,g:h,gimme:f}=t,b=[e];s=s||r||"",s=o?s:j(s),s&&(e.innerHTML=s),Object.assign(e,i||c),Object.assign(e.style,l||v);let m=f||h||[];if(m&&m.length)for(let E of m)b.push(e.querySelector(E));return Object.entries(d||{}).forEach(([E,_])=>e.addEventListener(E,_)),Object.entries(n||a||{}).forEach(([E,_])=>e.setAttribute(E,_)),b},S=(e,t={})=>{let{tag:s,id:r,classes:i}=A(e);if(i?.some(l=>l.includes("#")))throw new Error("Error: Found # in a class name. Did you mean to do elt#id.classes instead of elt.classes#id?");s||(s="div");let c=document.createElement(s);return r&&(c.id=r),(i||[]).forEach(l=>c.classList.add(l)),M(c,t)};var j=e=>e?e.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;").replace(/'/g,"&#39;"):"";var g=(e,t=document)=>t.querySelector(e),O=(e,t=document)=>Array.from(t.querySelectorAll(e));var I=e=>{var t;let s=e?e.match(/([0-9a-zA-Z\-]*)?(#[0-9a-zA-Z\-]*)?((.[0-9a-zA-Z\-]+)*)/):void 0,r=s?(t=s.slice(1,4))===null||t===void 0?void 0:t.map(i=>i?i.trim():void 0):Array(3).fill(void 0);return r&&r[1]&&(r[1]=r[1].replace(/#*/g,"")),s?{tag:r[0]||void 0,id:r[1]||void 0,classes:r[2]?r[2].split(".").filter(i=>i.trim()):void 0}:{}},L=(e,t={})=>{let{contents:s,c:r,misc:i,m:c,style:l,s:v,on:d,attrs:n,a,raw:o}=t;return s=s||r||"",s=o?s:y(s),e.innerHTML=s,Object.assign(e,i||c),Object.assign(e.style,l||v),Object.entries(d||{}).forEach(([h,f])=>e.addEventListener(h,f)),Object.entries(n||a||{}).forEach(([h,f])=>e.setAttribute(h,f)),e},C=(e,t={})=>{let{tag:s,id:r,classes:i}=I(e);s||(s="div");let c=document.createElement(s);return r&&(c.id=r),(i||[]).forEach(l=>c.classList.add(l)),L(c,t)},H=(e,t)=>{if(Object.keys(t).length!==1)throw new Error("Too many or too few positions specified.");let s=Object.values(t)[0],r="afterend";return t.after?r="afterend":t.before?r="beforebegin":t.atStartOf?r="afterbegin":t.atEndOf&&(r="beforeend"),s.insertAdjacentElement(r,e),e},x=class{constructor(e){this.value=null,this._subscribers={},this._subscriberCounts={},this._dead=!1,this.value=e}on(e,t,s=!1){return this._subscriberCounts[e]=this._subscriberCounts[e]||0,this._subscribers[e]=this._subscribers[e]||{},this._subscribers[e][this._subscriberCounts[e]]=t,s&&!["push","remove","mutation","setAt"].includes(e)&&t(this.value),this._subscriberCounts[e]++}unsubscribe(e,t){delete this._subscribers[e][t]}update(e){this._dead||(this.value=e,this._sendEvent("update",e))}refresh(){this._sendEvent("refresh",this.value)}_sendEvent(e,t){if(!this._dead){this._subscribers[e]=this._subscribers[e]||{};for(let s in Object.keys(this._subscribers[e]))this._subscribers[e][s](t)}}dispose(){this._dead=!0,this._subscribers={},this._subscriberCounts={}}},q=class extends x{constructor(e){super(e)}clear(){this.update([])}push(e){this.value.push(e),this._sendEvent("push",{value:e,idx:this.value.length-1})}remove(e){if(e<0||e>=this.value.length)throw new RangeError("Invalid index.");this._sendEvent("remove",{value:this.value.splice(e,1)[0],idx:e})}get(e){if(e<0||e>this.value.length)throw new RangeError("Invalid index.");return this.value instanceof Array&&this.value[e]}setAt(e,t){if(e<0||e>=this.value.length)throw new RangeError("Invalid index.");this.value[e]=t,this._sendEvent("mutation",{value:t,idx:e})}get length(){return this.value.length}},$=(e,t={})=>{let s=new RegExp("\\\\({{\\s*"+Object.keys(t).join("|")+"\\s*}})","gi");return new RegExp(Object.keys(t).join("|"),"gi"),e.replace(new RegExp("(^|[^\\\\]){{\\s*("+Object.keys(t).join("|")+")\\s*}}","gi"),function(r,i,c){return`${i||""}${t[c]}`}).replace(s,"$1")},R=(e,t={},s=!0)=>{let r=Object.assign({},t);return s&&(r=Object.fromEntries(Object.entries(r).map(([i,c])=>[i,y(c)]))),$(e,r)},D=(e,t)=>s=>R(e,s,t),y=e=>e?e.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;").replace(/'/g,"&#39;"):"",P=e=>{if(!e)return"";let t=/&(?:amp|lt|gt|quot|#(0+)?39);/g,s={"&amp;":"&","&lt;":"<","&gt;":">","&quot;":'"',"&#39;":"'"};return e.replace(t,r=>s[r]||"'")},u={Store:x,ListStore:q,nu:C,mustache:R,template:D,escape:y,unescape:P,extend:L,insert:H},z={display:"flex",alignItems:"center",justifyContent:"center",background:"rgba(0,0,0,0.6)",width:"100vw",height:"100vh",padding:0,margin:0,position:"fixed",top:0,left:0},w=()=>{var e;(e=document.querySelector("#alert-mask"))==null||e.remove();let t=u.insert(u.nu("div#alert-mask",{style:z,raw:!0,c:`<div id='cf-alert'>
        <div id='alert-header'></div>
        <div id='alert-body'></div>
        <div id='alert-footer'></div>
        </div>
        `}),{atEndOf:document.body}),s=t.querySelector("#alert-header"),r=t.querySelector("#alert-body"),i=t.querySelector("#alert-footer");return[s,r,i,t]};function Z(e,t="Error",s=!0){let[r,i]=w();return new Promise(()=>{r.innerHTML=u.escape(t),i.innerHTML=s?u.escape(e):e})}function F(e,t="Info",s="OK",r=!0){let[i,c,l,v]=w();return new Promise((d,n)=>{try{i.innerHTML=u.escape(t),c.innerHTML=r?u.escape(e):e,u.insert(u.nu("button#alert-ok",{on:{click:a=>{d(),v.remove()}},c:s}),{atEndOf:l})}catch(a){n(a)}})}function N(e,t,s="Input",r=!0){let[i,c,l,v]=w();return new Promise((d,n)=>{try{i.innerHTML=u.escape(s),c.innerHTML=r?u.escape(t):t;let a=u.insert(u.nu(`${e==="textarea"?"textarea":"input"}#alert-input`,{attrs:{type:e}}),{atEndOf:c});u.insert(u.nu("button#alert-cancel",{on:{click:o=>{n("Cancelled by user"),v.remove()}},c:"Cancel"}),{atEndOf:l}),u.insert(u.nu("button#alert-ok",{on:{click:o=>{d(a.value),v.remove()}},c:"Submit"}),{atEndOf:l})}catch(a){n(a)}})}function U(e,t,s="Are you sure?",r=!0){let[i,c,l,v]=w();return new Promise((d,n)=>{try{i.innerHTML=u.escape(s),c.innerHTML=r?u.escape(e):e,u.insert(u.nu("button#alert-cancel",{on:{click:a=>{d(!1),v.remove()}},c:t?.no||"No"}),{atEndOf:l}),u.insert(u.nu("button#alert-ok",{on:{click:a=>{d(!0),v.remove()}},c:t?.yes||"Yes"}),{atEndOf:l})}catch(a){n(a)}})}var p={input:N,message:F,confirm:U,fatal:Z};var B="succ",W="essfully",T=B+W;function Y(e){return e<=-100?0:e>=-50?100:Math.round((e- -100)/50*100)}var G=async()=>{let e=await fetch("/scan");return e.ok?(await e.json())?.scan_results||[]:(await p.message("Error fetching Wi-Fi scan results. Please enter details manually."),[])};window.addEventListener("DOMContentLoaded",async()=>{let e=O("input[id^=secret-token]"),t=g("#wifi-ssid"),s=g("#wifi-psk"),r=g("#username"),i=(n,a=!0,o,h)=>{let f=a?console.warn:p.message;if(!n||!n.includes(" "))return;let b=n.split(" ");if(b.length!==4)return o&&(o.value=""),f("Invalid secret. The secret phrase must have four words.","Error");if(b.some(m=>m.length<6||m.length>10))return o&&(o.value=""),f("The parts of the secret should be between 6 and 10 characters long.");h?.preventDefault(),b.forEach((m,E)=>e[E].value=m)};try{let n=b=>b.ok?b.json().then(m=>m.message==="ok"?m.value.trim():""):"",a=await fetch("/secret").then(n),o=await fetch("/psk").then(n),h=await fetch("/ssid").then(n),f=await fetch("/username").then(n);a&&i(a),o&&(s.value=o),h&&(t.value=h),f&&(r.value=f)}catch{}let c=await G().then(n=>n.toSorted((a,o)=>o.rssi-a.rssi));c.length||g(".networks-container-label")?.setHTMLUnsafe("No available networks"),g("section.networks")?.append(...c.map(n=>{let[a]=S("article.network",{raw:!0,on:{click:o=>{t.value=n.ssid}},c:k`
            <header>
                <span class="hidden">Network name</span>
                <strong>${n.ssid}</strong>
                <em>(${n.band})</em>
            </header>
            <div class="network-meta">
                <span class="hidden">Network security</span>
                    <span class="network-security">🔒 ${n.security}</span>
            </div>
            <div class=network-meta>
                <span class="hidden">Network strength</span>
                <span class="network-strength">📶 ${Y(n.rssi)}%</span>
            </div>
            `});return a})),e.forEach((n,a)=>{n.addEventListener("input",o=>{let h=n.value.endsWith(" ");n.value=n.value.trim(),h&&(o.preventDefault(),a<3&&e[a+1].focus())}),n.addEventListener("keyup",o=>{o.key==="Backspace"&&!n.value.trim()&&a>0&&e[a-1].focus()}),n.addEventListener("paste",o=>{let h=o.clipboardData?.getData("text/plain").trim();i(h,!0,n,o)})});let l=g("#secret-token-submit"),v=g("#wifi-submit"),d=async(n,a)=>{let o=await fetch(n,{method:"POST",body:new URLSearchParams({value:a}).toString(),headers:{"Content-Type":"application/x-www-form-urlencoded"}});if(!o.ok)throw await p.message("Error communicating with device: "+o.statusText,"Error"),new Error};l?.addEventListener("click",async()=>{if(!r.value.trim())return p.message("You must enter your username.");if(e.some(n=>!n.value.trim()))return p.message("All secret fields must be filled.");if(e.some(n=>n.value.length<6||n.value.length>10))return p.message("Secret parts must be between 6 and 10 characters long.");try{await d("/secret",e.map(n=>n.value.trim()).join(" ")),await d("/username",r.value.trim()),await p.message(`Secret changed ${T}.`)}catch{}}),v?.addEventListener("click",async()=>{if(!t.value.trim()||!s.value.trim())return p.message("Enter both the network name and password.");try{await d("/psk",s.value.trim()),await d("/ssid",t.value.trim()),await p.message(`Details changed ${T}.`)}catch{}})});})();
</script>
</body>
</html>
)====";

#endif
