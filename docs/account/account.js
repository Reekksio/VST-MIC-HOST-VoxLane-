'use strict';
const status=document.querySelector('#status');
let cfg,access='';
function show(text){status.textContent=text;}
async function request(path,body,method='POST'){
 if(!cfg)throw Error('Strona konta jest jeszcze ładowana. Spróbuj za chwilę.');
 const r=await fetch(cfg.supabaseUrl.replace(/\/$/,'')+path,{method,headers:{'Content-Type':'application/json',apikey:cfg.supabasePublicKey,...(access?{Authorization:'Bearer '+access}:{})},body:JSON.stringify(body)});
 const value=await r.json();
 if(!r.ok)throw Error(value.msg||value.error_description||value.error||'Operacja nie powiodła się.');
 return value;
}
function values(){return {email:document.querySelector('#email').value.trim(),password:document.querySelector('#password').value};}
async function action(fn){
 for(const b of document.querySelectorAll('button'))b.disabled=true;
 try{await fn();}catch(e){show(e.message);}finally{for(const b of document.querySelectorAll('button'))b.disabled=false;}
}
fetch('public-config.json',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error('Brak konfiguracji');return r.json();}).then(c=>{if(!c.supabaseUrl.startsWith('https://'))throw Error('Wymagane HTTPS');cfg=c;}).catch(()=>show('Strona konta nie jest jeszcze skonfigurowana.'));
document.querySelector('#account').onsubmit=e=>{e.preventDefault();action(async()=>{
 const s=await request('/auth/v1/token?grant_type=password',values());access=s.access_token;
 document.querySelector('#password').value='';show('Zalogowano. Teraz możesz wrócić do aplikacji VoxLane.');
});};
document.querySelector('#signup').onclick=()=>action(async()=>{
 const v=values();if(v.password.length<10)throw Error('Hasło musi mieć co najmniej 10 znaków.');
 await request('/auth/v1/signup',v);document.querySelector('#password').value='';show('Sprawdź pocztę i potwierdź adres e-mail.');
});
document.querySelector('#forgot').onclick=()=>action(async()=>{
 const email=values().email;if(!email)throw Error('Podaj adres e-mail.');
 await request('/auth/v1/recover',{email});show('Jeśli konto istnieje, otrzymasz wiadomość z linkiem do zmiany hasła.');
});
const hash=new URLSearchParams(location.hash.slice(1));
const link={token:hash.get('token_hash'),type:hash.get('type')};
if(link.token&&['signup','recovery'].includes(link.type)){
 // Opening or scanning the email link must not consume its one-time token.
 history.replaceState(null,'',location.pathname+location.search);
 document.querySelector('#account').hidden=true;
 document.querySelector('#email-action').hidden=false;
 document.querySelector('#email-action-title').textContent=link.type==='recovery'?'Zmień hasło VoxLane':'Potwierdź adres e-mail';
 document.querySelector('#verify-email').textContent=link.type==='recovery'?'Otwórz reset hasła':'Potwierdź adres';
 show('Kliknij przycisk, aby kontynuować.');
}else if(hash.get('type')==='recovery'&&hash.get('access_token')){
 // Existing emails from the previous template remain supported.
 access=hash.get('access_token');history.replaceState(null,'',location.pathname);
 document.querySelector('#recovery').hidden=false;document.querySelector('#account').hidden=true;show('Ustaw nowe hasło.');
}else{
 if(hash.has('access_token')||hash.has('refresh_token')||hash.has('error'))history.replaceState(null,'',location.pathname+location.search);
 if(hash.get('type')==='signup'&&hash.has('access_token'))show('Adres e-mail potwierdzony. Zaloguj się w aplikacji VoxLane.');
 if(hash.has('error'))show('Link jest nieprawidłowy lub wygasł. Poproś o nową wiadomość.');
 const payment=new URLSearchParams(location.search).get('payment');
 if(payment==='success')show('Wróć do aplikacji i naciśnij Sprawdź zakup. Potwierdzenie płatności może chwilę potrwać.');
 if(payment==='cancelled')show('Płatność anulowana. Możesz nadal korzystać z konta Free.');
}
document.querySelector('#email-action').onsubmit=e=>{e.preventDefault();action(async()=>{
 const s=await request('/auth/v1/verify',{token_hash:link.token,type:link.type});
 if(!s.access_token||!s.user?.id)throw Error('Nie udało się potwierdzić linku. Poproś o nową wiadomość.');
 link.token='';document.querySelector('#email-action').hidden=true;
 if(link.type==='recovery'){
  access=s.access_token;document.querySelector('#recovery').hidden=false;show('Ustaw nowe hasło.');
 }else{
  access='';document.querySelector('#account').hidden=false;show('Adres e-mail potwierdzony. Zaloguj się w aplikacji VoxLane.');
 }
});};
document.querySelector('#email-action-cancel').onclick=()=>{
 link.token='';document.querySelector('#email-action').hidden=true;document.querySelector('#account').hidden=false;show('Podaj adres e-mail, aby zalogować się lub poprosić o nowy link resetu hasła.');
};
document.querySelector('#recovery').onsubmit=e=>{e.preventDefault();action(async()=>{
 const password=document.querySelector('#new-password').value;if(password.length<10)throw Error('Hasło musi mieć co najmniej 10 znaków.');
 await request('/auth/v1/user',{password},'PUT');document.querySelector('#new-password').value='';access='';
 show('Hasło zmienione. Zaloguj się ponownie w VoxLane.');document.querySelector('#recovery').hidden=true;document.querySelector('#account').hidden=false;
});};
