'use strict';
const status=document.querySelector('#status');let cfg,access='';
function show(text){status.textContent=text;}
async function request(path,body,method='POST'){
 if(!cfg)throw new Error('Strona konta nie jest jeszcze skonfigurowana.');
 const r=await fetch(cfg.supabaseUrl.replace(/\/$/,'')+path,{method,headers:{'Content-Type':'application/json',apikey:cfg.supabasePublicKey,...(access?{Authorization:'Bearer '+access}:{})},body:JSON.stringify(body)});
 const value=await r.json();if(!r.ok)throw new Error(value.msg||value.error_description||value.error||'Operacja nie powiodła się');return value;
}
function values(){return {email:document.querySelector('#email').value.trim(),password:document.querySelector('#password').value};}
async function action(fn){for(const b of document.querySelectorAll('button'))b.disabled=true;try{await fn();}catch(e){show(e.message);}finally{for(const b of document.querySelectorAll('button'))b.disabled=false;}}
fetch('public-config.json',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error('Brak public-config.json');return r.json();}).then(c=>{if(!c.supabaseUrl.startsWith('https://'))throw Error('Wymagane HTTPS');cfg=c;}).catch(()=>show('Właściciel musi skonfigurować stronę konta.'));
document.querySelector('#account').onsubmit=e=>{e.preventDefault();action(async()=>{const s=await request('/auth/v1/token?grant_type=password',values());access=s.access_token;document.querySelector('#password').value='';show('Zalogowano. Teraz możesz wrócić do aplikacji VoxLane.');});};
document.querySelector('#signup').onclick=()=>action(async()=>{const v=values();if(v.password.length<10)throw Error('Hasło musi mieć co najmniej 10 znaków.');await request('/auth/v1/signup',v);document.querySelector('#password').value='';show('Sprawdź pocztę i potwierdź adres e-mail.');});
document.querySelector('#forgot').onclick=()=>action(async()=>{const email=values().email;if(!email)throw Error('Podaj adres e-mail.');await request('/auth/v1/recover',{email});show('Jeśli konto istnieje, otrzymasz wiadomość z linkiem do zmiany hasła.');});
const hash=new URLSearchParams(location.hash.slice(1));if(hash.get('type')==='recovery'&&hash.get('access_token')){access=hash.get('access_token');history.replaceState(null,'',location.pathname);document.querySelector('#recovery').hidden=false;document.querySelector('#account').hidden=true;show('Ustaw nowe hasło.');}else{if(hash.has('access_token')||hash.has('refresh_token'))history.replaceState(null,'',location.pathname+location.search);const p=new URLSearchParams(location.search).get('payment');if(p==='success')show('Wróć do aplikacji i naciśnij Sprawdź zakup. Potwierdzenie płatności może chwilę potrwać.');if(p==='cancelled')show('Płatność anulowana. Możesz nadal korzystać z konta Free.');}
document.querySelector('#recovery').onsubmit=e=>{e.preventDefault();action(async()=>{await request('/auth/v1/user',{password:document.querySelector('#new-password').value},'PUT');document.querySelector('#new-password').value='';access='';show('Hasło zmienione. Zaloguj się ponownie w VoxLane.');document.querySelector('#recovery').hidden=true;document.querySelector('#account').hidden=false;});};
